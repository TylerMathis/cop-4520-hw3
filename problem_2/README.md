# Problem 2
To run, simply run the makefile with `make`, or compile and run yourself:

```sh
g++ main.cpp -std=c++17 -o main -pthread
./main
rm main
```

The program will output all relevant data for each hour into a file called `results.txt`.

NOTE: A time scalar will be asked for at runtime, larger values will make the simulation run more quickly.

## Proof of Correctness
The generall idea behind this approach is to store all of our tempurature readings in a shared vector, so that it is easy to spin up an analysis thread to analyze the data after an "hour" has passed. In order to ensure that no thread is overwriting data, we utilize an atomic shared index that is incremented on each write. In order to ensure that each thread is taking it's readings at the same time, and no thread is getting ahead, we implement a condition variable that will wake up all write jobs at the same time for writing. Once the array is fully populated, it is time to analyze the data.

A single thread is then used to analyze the data. This thread runs concurrently with future data collection. We acquire all of the stats needed by first conducting a sliding window sweep to look for our largest difference. Then, we sort the data to acquire the min and max 5 elements.

## Efficiency Analysis
Efficiency analysis of the collection method is rather useless, as it is simply generating random numbers within our bound. This is an O(1) operation. The largest temp difference window runs in O(n), as we simply check every window, and then the 5 mins and maxes runs in O(nlogn) as we need to sort first. Sorting is not strictly required, but for the scope of this problem it is more than reasonable.

## Experimental Evaluation
- Multiple time scalars were tested.
- Data verification by hand