# MultithreadedHybridQuicksort
Multithreaded and Hybrid Quicksort demonstration for EECS3540: Systems Programming.

This project showcases a multithreaded and hybrid quicksort algorithm for massive lists on a Linux system. The user has a choice between using Insertion or Shell sort past a user-set threshold instead of continuing with the quicksort, decreasing sort time.
For testing, a typical command takes this form:

./*executable name* -n *size of list to sort* 

Other user-specified optional parameters
-s: threshold at which to switch to alternate sort
-a: specify either s for shell sort or i for insertion sort. Shell sort is the default.
-r: specify a seed for the random shuffling algorithm
-m: specify y for multithreaded capabilities, n for no multithreading. y is default.
-p: pieces to partition the list into before beginning multithreaded quicksort. default is 10.
-t: maximum number of threads to use for sorting.
-m3: whether to use median of three in the quicksort partition algorithm, y or n. default is n.
