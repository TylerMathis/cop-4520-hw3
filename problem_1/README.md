# Problem 2
To run, simply run the makefile with `make`, or compile and run yourself:

```sh
javac BirthdayPresentsParty.java;
java -ea BirthdayPresentsParty
rm *.class;
```

NOTE: To see debug information about the Minotaur asking about presents in the chain, run with the "-verbose" flag.
`make verbose`
or
`java -ea BirthdayPresentsParty -verbose`

## Proof of Correctness
The correctness of this implementation can be broken into several parts:

1. Correctness of the LockFreeLinkedList:
	Outlined in chapter 9 of our textbook: The Art of Multiprocessor Programming.
2. Correctnes of the chain add/remove protocal:
	In order to ensure that we have added a present before we remove it, we will take a two pointer approach. After generating a random permutation of presents, we will create two pointers, the `addPointer` and the `removePointer`. The `addPointer` will always be in front of the `removePointer`. This ensures that the item that we are attempting to remove exists, aside from the span of time it takes to actually add an item. To account for this time we simply spin until the item exists before removing it. This spin will only iterate a few time, as we are waiting for an addition to finish, which we guarantee has already started.


## Efficiency Analysis
The efficiency is a simple `O(n*t)` where `n` is the present count and `t` is the thread count. This is because each element will need to inserted and deleted, each of which can take a maximum time of `O(list_len)`. Since our list can only grow to a worst case length of `t`, our runtime is bound tightly.

## Experimental Evaluation
- Assertions are included after the main runtime to assure that all thank you notes have been written
- Tested on small cases, large cases, and with variable thread counts
