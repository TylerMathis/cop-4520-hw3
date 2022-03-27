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

using std::atomic;
using std::mutex;
using std::unique_lock;
using std::condition_variable;
using std::vector;
using std::thread;
using std::sort;
using std::cout;
using std::min; using std::max;

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

int main() {
	// Seed our num gen
	srand(time(NULL));

	// Hold all readings recorded per cycle
	int readings[DATA_POINTS];

	atomic<int> initCnt(0);
	auto tempWriteJob = [&](int threadID) {
		int curIter = 0;
		bool isInit = false;
		while (curIter < READINGS_PER_CYCLE) {
			{
				unique_lock lk(mtx);
				if (!isInit) {
					isInit = true;
					initCnt++;
				}
				cv.wait(lk);
			}

			int temp = (rand() % TEMP_UPPER + 1) - abs(LOW_TEMP);
			readings[(curIter++ * SENSORS) + threadID] = temp;
		}
	};

	vector<thread> jobs;
	for (int i = 0; i < SENSORS; i++)
		jobs.emplace_back(tempWriteJob, i);
	while (initCnt < SENSORS);

	// Take readings 10 times a second
	for (int i = 0; i < READINGS_PER_CYCLE; i++) {
		cv.notify_all();
		cout << "All sensors reading at time = " << i+1 << "\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	for (auto &t : jobs)
		t.join();

	// Sort and get useful info
	sort(readings, readings + DATA_POINTS);
	cout << "MIN 5: ";
	for (int i = 0; i < 5; i++)
		cout << readings[i] << "F ";
	cout << "\n";
	cout << "MAX 5: ";
	for (int i = DATA_POINTS - 6; i < DATA_POINTS; i++)
		cout << readings[i] << "F ";
	cout << "\n";

	int largestDiff = INT_MIN, largestDiffIdx = -1;
	for (int i = 0; i < DATA_POINTS - (DIFF_INTERVAL * SENSORS); i++) {
		int minTemp = INT_MAX, maxTemp = INT_MIN;
		for (int j = i; j < i + (DIFF_INTERVAL * SENSORS); j++) {
			minTemp = min(minTemp, readings[j]);
			maxTemp = max(maxTemp, readings[j]);
		}
		int diff = maxTemp - minTemp;
		if (diff > largestDiff) {
			largestDiff = diff;
			largestDiffIdx = i;
		}
	}

	cout << "Largest difference window from [" <<
		largestDiffIdx / SENSORS << ", " <<
		(largestDiffIdx + (DIFF_INTERVAL * SENSORS)) / SENSORS <<
		"]\n";

    return 0;
}

