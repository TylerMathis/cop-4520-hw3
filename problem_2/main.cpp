#include <iostream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <limits>
#include <climits>
#include <fstream>
#include <string.h>

using std::atomic;
using std::mutex;
using std::unique_lock;
using std::condition_variable;
using std::vector;
using std::thread;
using std::sort;
using std::cin; using std::cout;
using std::min; using std::max;
using std::ofstream;

// Problem constants
const int SENSORS = 8;
const int READINGS_PER_CYCLE = 60;
const int DATA_POINTS = SENSORS * READINGS_PER_CYCLE;
const int DIFF_INTERVAL = 10;
const int LOW_HI_BOUND = 5;
const int LOW_TEMP = -100;
const int HI_TEMP = 70;
const int TEMP_UPPER = abs(LOW_TEMP) + HI_TEMP;

// Global sync
mutex mtx;
condition_variable cv;

int main(int argc, char **argv) {
	// Parse command-line args
	bool unique_bounds = false;
	if (argc > 1 && strcmp(argv[1], "-unique") == 0)
		unique_bounds = true;

	// Seed our num gen
	srand(time(NULL));

	// Ask for time multiplier
	double timeMultiplier;
	do {
		cout << "Please enter a positive time multiplier <= 10,000: ";
		cin >> timeMultiplier;
	} while (timeMultiplier <= 0 || timeMultiplier > 10'000);

	// Ask for num hours
	int numHours;
	do {
		cout << "Please enter a positive number of hours to run for: ";
		cin >> numHours;
	} while (numHours <= 0);

	// One minute / multiplier
	int sleepMillis = (1000 * 60) / timeMultiplier;

	// Empty out results
	ofstream out("results.txt");
	out.close();


	// Hold all readings recorded per cycle
	int readings[2][DATA_POINTS];

	// We need to only start the program once our sensors have spun up
	atomic<int> initCnt(0);
	// Keep our current hour for dataframe swapping
	atomic<int> curHour(0);
	// Whether or not we should exit
	atomic<bool> done(false);

	// This is our main sensor simulator
	// It waits for a notification from the main thread to take a reading,
	// then does so and stores it's data in the log
	auto tempWriteJob = [&](int threadID) {
		while (true) {
			int curIter = 0;
			bool isInit = false;
			while (curIter < READINGS_PER_CYCLE) {
				{
					unique_lock<decltype(mtx)> lk(mtx);
					// Let main know that we are ready
					if (!isInit) {
						isInit = true;
						initCnt++;
					}
					// Wait for notifications
					cv.wait(lk);

					// If we're done, stop
					if (done)
						return;
				}

				// Take the reading!
				int temp = (rand() % (TEMP_UPPER + 1)) - abs(LOW_TEMP);
				readings[curHour % 2][(curIter++ * SENSORS) + threadID] = temp;
			}
		}
	};

	auto tempAnalyzeJob = [&](int hour) {
		// Open in append mode
		out.open("results.txt", std::ios_base::app);

		out << "Data log for hour: " << hour+1 << "\n";
		int dataSeg = hour % 2;

		// Find window of largest difference before sorting
		int largestDiff = INT_MIN, largestDiffIdx = -1;
		for (int i = 0; i < DATA_POINTS - (DIFF_INTERVAL * SENSORS); i++) {
			int minTemp = INT_MAX, maxTemp = INT_MIN;
			for (int j = i; j < i + (DIFF_INTERVAL * SENSORS); j++) {
				minTemp = min(minTemp, readings[dataSeg][j]);
				maxTemp = max(maxTemp, readings[dataSeg][j]);
			}
			int diff = maxTemp - minTemp;
			if (diff > largestDiff) {
				largestDiff = diff;
				largestDiffIdx = i;
			}
		}

		// Sort and get mins and maxes
		sort(readings[dataSeg], readings[dataSeg] + DATA_POINTS);
		vector<int> lows, highs;
		if (unique_bounds) {
			int curLow = INT_MIN, curHigh = INT_MAX;
			for (int i = 0; i < DATA_POINTS; i++) {
				if (readings[dataSeg][i] > curLow) {
					curLow = readings[dataSeg][i];
					lows.push_back(curLow);
					if (lows.size() >= LOW_HI_BOUND)
						break;
				}
			}

			for (int i = DATA_POINTS - 1; i >= 0; i--) {
				if (readings[dataSeg][i] < curHigh) {
					curHigh = readings[dataSeg][i];
					highs.push_back(curHigh);
					if (highs.size() >= LOW_HI_BOUND)
						break;
				}
			}
			reverse(highs.begin(), highs.end());
		} else {
			for (int i = 0; i < LOW_HI_BOUND; i++)
				lows.push_back(readings[dataSeg][i]);
			for (int i = DATA_POINTS - LOW_HI_BOUND; i < DATA_POINTS; i++)
				highs.push_back(readings[dataSeg][i]);
		}

		out << "MIN " << LOW_HI_BOUND << ": ";
		for (int t : lows)
			out << t << "F ";
		out << "\n";
		out << "MAX " << LOW_HI_BOUND << ": ";
		for (int t : highs)
			out << t << "F ";
		out << "\n";

		// Output difference window
		out << "Largest difference window of " << largestDiff <<
			"F detected in minute range [" <<
			largestDiffIdx / SENSORS << ", " <<
			(largestDiffIdx + (DIFF_INTERVAL * SENSORS)) / SENSORS <<
			"]\n\n";

		// Flush and close output
		out.close();
	};

	// Start all sensors and wait for them to spin up
	vector<thread> jobs, analyzers;
	for (int i = 0; i < SENSORS; i++)
		jobs.emplace_back(tempWriteJob, i);
	while (initCnt < SENSORS);

	// Main temp loop
	while (curHour < numHours) {
		// Take all readings
		for (int i = 0; i < READINGS_PER_CYCLE; i++) {
			cv.notify_all();
			cout << "\r";
			cout.flush();
			cout << "Taking reading at time: " << curHour+1 << ":" << i+1;
			cout.flush();
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepMillis));
		}
		cout << "\nLogging results for hour " << curHour+1 << " to file\n";

		// Boot up the analyzer and let it run
		analyzers.emplace_back(tempAnalyzeJob, curHour.load());
		curHour++;
	}

	// Clean up the threads
	cout << "Cleaning up...\n";
	done = true;
	cv.notify_all();
	for (thread &t : jobs)
		t.join();
	for (thread &t : analyzers)
		t.join();

	cout << "Done! All results available in './results.txt'\n";

    return 0;
}

