# Problem 2
To run, simply run the makefile with `make`, or compile and run yourself:

```sh
g++ main.cpp -std=c++17 -o main -pthread
./main
rm main
```

There was a discrepency in the documentation and in the class Discord chat about whether or not our mins and maxs were supposed to be on unique values or all values, so this can be specified when `make`ing the application. Simply run with `make unique`, to force mins and maxs to compare on only unique elements when conducting analysis.

The program will output all relevant data for each hour into a file called `results.txt`.

NOTE: A time scalar will be asked for at runtime, larger values will make the simulation run more quickly. A time scalar of `1` means time flows normally, one minute between readings. `2` means that the simulation will run twice as fast. For ease of grading, I would recommend the max time scalar of `10,000`.

## Proof of Correctness
The general idea behind this approach is to store all of our tempurature readings in a shared vector, so that it is easy to spin up an analysis thread to analyze the data after an "hour" has passed. In order to ensure that no thread is overwriting data, we utilize an atomic shared index that is incremented on each write. In order to ensure that each thread is taking it's readings at the same time, and no thread is getting ahead, we implement a condition variable that will wake up all write jobs at the same time for writing. Once the array is fully populated, it is time to analyze the data. We allow the next iteration to begin immediately by keeping two copies of the relevant data, and only reading from the version that is not being written to.

A single thread is then used to analyze the data. Note that this is a ninth thread at execution time, however, the spec is once again ambiguous on whether or not this should be accepted. It states that the analysis should be done by a completely separate "atomospheric temperature module", which implies that it should not be represented by one of the tempurature sensor threads. If this thread were truly not allowed in the spec of this assignment, it would be easy to make the readers wait while analysis is performed on all 8 sensor threads, and then move directly back into the recording task. This "atmospheric temperature module" thread runs concurrently with future data collection. We acquire all of the stats needed by first conducting a sliding window sweep to look for our largest difference. Then, we sort the data to acquire the min and max 5 elements.

## Efficiency Analysis
Efficiency analysis of the collection method is rather useless, as it is simply generating random numbers within our bound. This is an O(1) operation. The largest temp difference window runs in O(n), as we simply check every window, and then the 5 mins and maxes runs in O(nlogn) as we need to sort first. Sorting is not strictly required, but for the scope of this problem it is more than reasonable.

## Experimental Evaluation
- Multiple time scalars were tested (unreasonable time scalars 100,000x and above are not guarnateed to function properly, as this does not fit the context of the problem, so we restrict to 10'000x)
- Data verification by hand
