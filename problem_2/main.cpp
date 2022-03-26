#include <iostream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <chrono>

using std::atomic;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;
using std::vector;
using std::thread;

// Problem constants
const int SENSORS = 8;
const int READINGS_PER_CYCLE = 60;
const int DIFF_INTERVAL = 10;
const int LOW_HI_BOUND = 5;
const int LOW_TEMP = -100;
const int HI_TEMP = 70;
const int TEMP_UPPER = abs(LOW_TEMP) + HI_TEMP;

// Global sync
mutex mtx;
condition_variable cv;

int main() {
	// Hold all readings recorded per cycle
	int readings[SENSORS * READINGS_PER_CYCLE];

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
			std::cout << "Sensor: " << threadID << " read " << temp << "\n";
			readings[(curIter++ * SENSORS) + threadID] = temp;
		}
	};

	vector<thread> jobs;
	for (int i = 0; i < SENSORS; i++)
		jobs.emplace_back(tempWriteJob, i);
	while (initCnt < SENSORS);

	for (int i = 0; i < READINGS_PER_CYCLE; i++) {
		cv.notify_all();
		std::this_thread::sleep_for(std::chrono::nanoseconds(100000000));
	}
	for (auto &t : jobs)
		t.join();

	for (int i = 0; i < SENSORS * READINGS_PER_CYCLE; i++)
		std::cout << readings[i] << ' ';
	std::cout << std::endl;

    return 0;
}

