import java.util.Random;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicMarkableReference;

class BirthdayPresentsParty {

	// Problem constants
	static final int NUM_SERVANTS = 4;
	static final int NUM_PRESENTS = 500_000;

	// Whether or not to print data about present queries
	static boolean verbose = false;

	// Unsorted back to be shared
	static AtomicInteger addIndex, thankYouIndex;
	static int[] unsortedBag = new int[NUM_PRESENTS];

	static LockFreeLinkedList presentChain;

	public static void main(String[] args) {

		// Check for verbose flag
		verbose = args.length > 0 && args[0].equals("-verbose");

		// Start our thank and add indicies at 0
		addIndex = new AtomicInteger(0);
		thankYouIndex = new AtomicInteger(0);

		// Populate bag with IDs
		for (int i = 0; i < NUM_PRESENTS; i++)
			unsortedBag[i] = i;

		// Randomly shuffle them
		Random rand = new Random();
		for (int i = 0; i < NUM_PRESENTS; i++) {
			int randToIdx = rand.nextInt(NUM_PRESENTS);
			int tmp = unsortedBag[i];
			unsortedBag[i] = unsortedBag[randToIdx];
			unsortedBag[randToIdx] = tmp;
		}

		// Create the present chain
		presentChain = new LockFreeLinkedList();

		System.out.println("Preparation ready, starting servants!");

		// Spawn all servants
		ArrayList<Thread> threads = new ArrayList<>();
		for (int i = 0; i < NUM_SERVANTS; i++) {
			Thread t = new Thread(new Servant(i+1));
			t.start();
			threads.add(t);
		}

		// Wait for them to finish
		for (Thread t : threads) {
			try {
				t.join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}

		System.out.println("Done with all thank you notes!");

		// Assert that all presents have been thanked
		for (int i = 0; i < NUM_PRESENTS; i++) {
			assert !presentChain.contains(i) : "Presents still left";
		}
	}

	// Main worker servant
	static class Servant implements Runnable {
		int id;
		Random rand;
		public Servant(int id) {
			this.id = id;
			this.rand = new Random();
		}

		// Run the servant
		public void run() {
			boolean shouldAdd = true;
			boolean doneAdding = false;

			// Until done
			while (true) {

				// Might randomly be asked if a present is in the chain
				// 1% chance
				if (rand.nextInt(100) == 0) {
					int randomCheck = rand.nextInt(NUM_PRESENTS);
					boolean exists = presentChain.contains(randomCheck);

					if (verbose)
						System.out.printf("The minotaur asked if present with ID "
							+ "%-6d"
							+ " was in the chain... it was "
							+ (exists ? "found!\n" : " not there :(\n"),
							randomCheck
						);
				}

				// Time to add
				if (shouldAdd && !doneAdding) {
					Integer myAddIndex = addIndex.getAndIncrement();

					// Note if we are done adding
					if (myAddIndex >= NUM_PRESENTS) {
						doneAdding = true;
						continue;
					}

					// Add our present, and swap tasks
					presentChain.add(unsortedBag[myAddIndex]);
					shouldAdd = !shouldAdd;
				}
				// Time to than
				else {
					Integer myThankIndex = thankYouIndex.getAndIncrement();

					// We are completely done, stop
					if (myThankIndex >= NUM_PRESENTS)
						return;

					// Wait until the present exists
					while (!presentChain.contains(unsortedBag[myThankIndex]));

					// Remove the present
					presentChain.remove(unsortedBag[myThankIndex]);
					shouldAdd = !shouldAdd;
				}
			}
		}
	}
}

// Lock-Free Linked List from chapter 9 in The Art of Multiprocessor Programming
class LockFreeLinkedList {

	// Standard Linked List node
	class Node {
		public int val;
		public AtomicMarkableReference<Node> next;

		Node(int val) {
			this.val = val;
			this.next = new AtomicMarkableReference<Node>(null, false);
		}
	}

	// Store a window of nodes surrounding target area.
	class Window {
		public Node pred, curr;
		Window(Node pred, Node curr) {
			this.pred = pred;
			this.curr = curr;
		}
	}

	// Head, and constructor
	private Node head;
	LockFreeLinkedList() {
		head = new Node(Integer.MIN_VALUE);
		head.next = new AtomicMarkableReference<Node>(
			new Node(Integer.MAX_VALUE), false
		);
	}

	// Return a Window containing the nodes on either side of our target
	// Removes marked nodes when found
	public Window find(Node head, int val) {
		Node pred = null, curr = null, succ = null;
		boolean[] marked = {false};
		boolean snip;
		retry: while (true) {
			pred = head;
			curr = pred.next.getReference();
			while (true) {
				succ = curr.next.get(marked);
				while (marked[0]) {
					snip = pred.next.compareAndSet(curr, succ, false, false);
					if (!snip) continue retry;
					curr = succ;
					succ = curr.next.get(marked);
				}
				if (curr.val >= val)
					return new Window(pred, curr);
				pred = curr;
				curr = succ;
			}
		}
	}

	// Keep attempting to add item to list, retrying on CAS failure
	public boolean add(int val) {
		while (true) {
			Window window = find(head, val);
			Node pred = window.pred, curr = window.curr;
			if (curr.val == val) {
				return false;
			} else {
				Node node = new Node(val);
				node.next = new AtomicMarkableReference<Node>(curr, false);
				if (pred.next.compareAndSet(curr, node, false, false)) {
					return true;
				}
			}
		}
	}

	// Logically remove item from list. If possible, physically delete as well
	public boolean remove(int val) {
		boolean snip;
		while (true) {
			Window window = find(head, val);
			Node pred = window.pred, curr = window.curr;
			if (curr.val != val) {
				return false;
			} else {
				Node succ = curr.next.getReference();
				snip = curr.next.compareAndSet(succ, succ, false, true);
				if (!snip)
					continue;
				pred.next.compareAndSet(curr, succ, false, false);
				return true;
			}
		}
	}

	// Return whether or not an item exists
	public boolean contains(int val) {
		boolean[] marked = {false};
		Node curr = head;
		while (curr.val < val) {
			curr = curr.next.getReference();
			curr.next.get(marked);
		}
		return (curr.val == val && !marked[0]);
	}
}
