/*
Multithreaded Quick-Sort
Nathaniel Green
03/08/2022 to 03/20/2022

This program showcases a multi-threaded hybrid quicksort implementation which can be used to sort large lists.
This includes many user options. The user can specify the size of list to be sorted, the threshold at which to use an alternate sort,
the type of alternate sort (Shell or Insertion), the seed for randomizing the list, whether or not to multithread,
the number of pieces to partition before multithread sorting starts, the maximum number of sorting threads to use, and whether or not
to use the Median of 3 method for partitioning. 

From a development point of view, the program can be split into 5 parts: Input Parsing, Array creation/randomization, 
Partition Handling, Quicksort, and Multithreading. By developing the program in that order, I was able to thouroughly test each part before proceeding,
ensuring a smooth development process throughout.

03/08 to 03/10 - Implemented input checking, array init/randomizing, partitioning, quicksort
03/11 - Implemented multithreading algorithm, debugging
03/12 to 03/15 - debugging, performance testing
03/17 to 03/20 - commenting, finalization, and script running
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>


int size = 0; //n
int threshold = 10; //s
char alternate = 's'; //a
int seed = 0; //r
char seedProvided = 'n';
char multithread = 'y'; //m
int pieces = 10; //p
int maxthreads = 4; //t
char median = 'n'; //m3
int* arr; // where all the numbers go
struct interval* pieceArr;

struct interval{
    int lower; // lower bound of interval
    int upper; // upper bound of interval
    int size; //this is obviously not necessary, but makes code below more readable and graceful
};

int partition(int lowerbound, int upperbound);
void quickSort(int lowerbound, int upperbound, int segmentSize);
void *runner(void *passed);

/*used in the switch cases below to collect integer input from argv and handle validation. 
Passed the variable to put the int in, the argument, and the lowest acceptable int.*/
void cmdIntInput(int* put, char* arg, int lowerBound){
    *put = atoi(arg);

    if( *put < lowerBound && &seed != put ){ //special case for seed, as it has no lower bound
        fprintf(stderr, "Invalid Input\n");
        exit(1);}
}

/*used in the switch cases below to collect character input from argv and handle validation. 
Passed the variable to put the char in, the argument, and the two acceptable alternatives*/
void cmdCharInput(char* put, char* arg, char altOne, char altTwo){
    *put = *arg | 0x20; //makes all letters lowercase 
    if((*put != altOne && *put != altTwo) || arg[1] != 0){ //checks the alternatives and makes sure no other letters follow
        fprintf(stderr, "Invalid Input\n");
        exit(1);}
}


int main(int argc, char *argv[]){
    //total time start
    struct timeval walltimetotal;
    gettimeofday(&walltimetotal, NULL);


    
    if(argc % 2 == 0){ // provided options but no params or vice versa
        fprintf(stderr, "Invalid Input\n");
        return 1;
    }

    int cmdlast;
    for(int g = 1; g < argc; g = g + 2){
        if(argv[g][0] != 0x2D){ // no dash at beginning of an option 
            fprintf(stderr, "Invalid Input\n");
            return 1;
        }

        cmdlast = 2;
        switch(argv[g][1] | 0x20){ // handles the letter past the dash
        case 'n': // -n
            cmdIntInput(&size, argv[g+1], 1);
            break;
        case 's': // -s
            cmdIntInput(&threshold, argv[g+1], 0);
            break;
        case 'a': // -a
            cmdCharInput(&alternate, argv[g+1], 'i', 's');
            break;
        case 'r': // -r
            cmdIntInput(&seed, argv[g+1], 0);
            seedProvided = 'y'; //seed provided, useful later
            break;
        case 'p': // -p
            cmdIntInput(&pieces, argv[g+1], 1);
            break;
        case 't': // -t
            cmdIntInput(&maxthreads, argv[g+1], 1);
            break;
        case 'm': // -m and -m3
            if(argv[g][2] == 0x33){//m has a special case with -m and -m3
                cmdCharInput(&median, argv[g+1], 'y', 'n');
                cmdlast = cmdlast + 1;}
            else{
                cmdCharInput(&multithread, argv[g+1], 'y', 'n');}
            break;
        default: //none of the above options were input
            fprintf(stderr, "Invalid Input\n");
            return 1;
            break;
        }
        if(argv[g][cmdlast] != 0){ // checks for null terminator at end of string. prevents the user from inputting -mmm for example
            fprintf(stderr, "Invalid Input\n");
            return 1;
        }
    }
    if(size < 1){ // checks if there wasn't a -n input
        fprintf(stderr, "Invalid Input\n");
        return 1;
    }
    if(maxthreads > pieces){ // ensures maxthreads can't be more than pieces
        fprintf(stderr, "Invalid Input\n");
        return 1;
    }

    //Input Parsing Complete

    clock_t clock1;

    //create array
    clock1 = clock();

    arr = (int *)malloc(sizeof(int)*size);//Dynamically sized global array on the heap

    clock1 = clock() - clock1;
    printf("Array created in %.3f seconds\n", ((float)clock1) / CLOCKS_PER_SEC);


    //initialize array
    clock1 = clock();
    for(int i = 0; i <size; i++){
        arr[i] = i;
    }

    clock1 = clock() - clock1;
    printf("Array initialized in %.3f seconds\n", ((float)clock1) / CLOCKS_PER_SEC);

    //array randomizing
    clock1 = clock();

    
    if(seed == -1){ // Taking care of -r input
        srand(time(NULL));
    }else if (seedProvided == 1){
        srand(seed);
    }

    int randInd; // will hold our random index
    int temp; //swap variable used multiple times throughout main

    for(int currInd = 0; currInd < size-1; currInd++){ //shuffling loop
        //No need to swap with indexes already guaranteed a swap. Does make the random number parsing more complex. 
        //just using % can create bias, wasn't sure how strict to be about this.
        randInd = currInd + (rand()/((RAND_MAX/(size-currInd))+1)); 
        temp = arr[currInd];
        arr[currInd] = arr[randInd];
        arr[randInd] = temp;
    }
    
    clock1 = clock() - clock1;
    printf("Array randomized in %.3f seconds\n", ((float)clock1) / CLOCKS_PER_SEC);
    
    //done randomizing, begin algorithm
    struct timeval walltime1;
    struct timeval walltime2;

    
    clock1 = clock();
    gettimeofday(&walltime1, NULL);


    if(multithread == 'n'){// No multithreading = start quicksort directly.
            quickSort(0, size-1, size);
    }else{ 

        int total = 1;
        int biggest;
        pieceArr = (struct interval*)malloc(sizeof(struct interval)*pieces); // Another dynamically sized array to hold our intervals / PIECES. Using MALLOC made my debugging cleaner.
        pieceArr[0].lower = 0; //initializing first interval as the entire array
        pieceArr[0].upper = size-1;
        pieceArr[0].size = size;

        for(int i = 0; i < pieces-1; i+= 1){//Loop to fill up pieceArray.
            biggest = 0;
            for(int bigsearch = 0; bigsearch < total; bigsearch += 1){ //Finds the biggest interval to break apart.
                if(pieceArr[biggest].size<pieceArr[bigsearch].size){
                    biggest = bigsearch;
                }
            }
            //splits the biggest interval and assigns its parts to its current index and a new one.
            //at the end of this block, the old partition will now be its original lower bound up to the pivot minus one,
            //and the new partition is the pivot plus one and the original upper bound.
            pieceArr[total].upper = pieceArr[biggest].upper;
            temp = partition(pieceArr[biggest].lower, pieceArr[biggest].upper); //the pivot doesn't need to be included in the new intervals.
            pieceArr[total].lower = temp+1;
            pieceArr[biggest].upper = temp-1;
            pieceArr[biggest].size = pieceArr[biggest].upper-pieceArr[biggest].lower+1;
            pieceArr[total].size = pieceArr[total].upper-pieceArr[total].lower+1;

            total += 1;
        }
        
        //sorting partitions by size, biggest first. Small list so insertion should be okay.
        //Algorithm from provided slides
        struct interval tempinterval;

        for(int i = 0; i < pieces; i += 1){
                tempinterval = pieceArr[i];
                int j;
                for(j = i - 1; j >= 0 && tempinterval.size > pieceArr[j].size; j -= 1){
                    pieceArr[j+1] = pieceArr[j];
                }
                pieceArr[j+1] = tempinterval;
            }



        //initializing important variables for threading below.
        int nextPart = 0;//next partition to be worked on
        int numIdle = 0;//number of idle worker threads
        pthread_t tid[maxthreads];//holds worker thread information
        pthread_attr_t attr;
        pthread_attr_init(&attr);


        for(int i = 0; i < maxthreads; i += 1){ // initially fill the worker threads
            pthread_create(&tid[i], &attr, runner, &pieceArr[i]);
            nextPart += 1;
        }

        while(numIdle != maxthreads){ //ends when all available workers are idle
            usleep(50000); //wait so as not to consume resources 
            for(int i = 0; i < maxthreads; i += 1){
                if(pthread_tryjoin_np(tid[i], NULL) == 0){//if a thread is finished:
                    if(nextPart != pieces){ //load another interval in if available.
                        pthread_create(&tid[i], &attr, runner, &pieceArr[nextPart]);
                        nextPart += 1;
                    }else{//if not, add to the number of idle workers
                        numIdle += 1;
                    }

                }
            }
            
        }

        
    }

    //timing printouts for sorting time and total time
    gettimeofday(&walltime2, NULL);
    clock1 = clock() - clock1;
    float walltime = (walltime2.tv_sec-walltime1.tv_sec)*1.0 + (walltime2.tv_usec-walltime1.tv_usec)/1000000.0;
    printf("Seconds spent sorting: Wall Clock:  %.3f / CPU: %.3f\n", walltime, ((float)clock1) / CLOCKS_PER_SEC);
    walltime = (walltime2.tv_sec-walltimetotal.tv_sec)*1.0 + (walltime2.tv_usec-walltimetotal.tv_usec)/1000000.0;
    printf("Total Run Time (sec): %.3f\n", walltime);
    


    //begin checking for errors. Simple i != arr[i] will detect any errors due to how we initialized the list.
    for(int i = 0; i < size; i += 1){
        if(i != arr[i]){
            fprintf(stderr, "Failure\n");
            return(1);
        }
    }
    return(0);
}


//partition for partitioning portion, returns the pivot point.
int partition(int lowerbound, int upperbound){//upper and lower bound both inclusive
    int temp;
    if(median == 'y'){//handling -m3 input
        int k = (lowerbound + upperbound) / 2;
        if(arr[lowerbound]>arr[k]){
            temp = arr[lowerbound];
            arr[lowerbound] = arr[k];
            arr[k] = temp;
        }
        if(arr[lowerbound]>arr[upperbound]){
            temp = arr[lowerbound];
            arr[lowerbound] = arr[upperbound];
            arr[upperbound] = temp;
        }
        if(arr[k]>arr[upperbound]){
            temp = arr[k];
            arr[k] = arr[upperbound];
            arr[upperbound] = temp;
        }
    }

    int i = lowerbound;
    int j = upperbound+1;
    int pivot = arr[lowerbound];
    //partition algorithm from slides:
    do{
        do{i += 1;}while(arr[i] < pivot);
        do{j -= 1;}while(arr[j] > pivot);
        if(i<j){
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }else{ break; }

    }while(1);
    temp = arr[lowerbound];
    arr[lowerbound] = arr[j];
    arr[j] = temp;
    return j;
}



void quickSort(int lowerbound, int upperbound, int segmentSize){
    int temp;
    if(segmentSize < 2){// segment isn't big enough to sort.
        return;
    }else if(segmentSize == 2){// segment may only need one swap, as it's two indexes long
        if(arr[lowerbound]>arr[upperbound]){
        temp = arr[lowerbound];
        arr[lowerbound] = arr[upperbound];
        arr[upperbound] = temp;
        }
        return;
    }else if(segmentSize <= threshold){ // segment size small enough to switch to the alternate sort.
        if(alternate == 's'){ //if s, shell sort algorithm w/ Hibbards from the slides:
            int k = 1;
            while ( k <= segmentSize ) k *= 2; k = (k / 2) - 1;
            do
            {
                for(int i = 0; i < (segmentSize - k); i += 1){
                    for(int j = i; j >= 0; j-= k){
                        if(arr[j + lowerbound] <= arr[j + k + lowerbound]){
                            break;
                        }else{
                            temp = arr[j + k + lowerbound];
                            arr[j + k + lowerbound] = arr[j + lowerbound];
                            arr[j + lowerbound] = temp;
                        }
                    }
                }
            k = k >> 1;
            }while (k>0);
            
        }else{ //if not s, then it's i, insertion sort from the slides:
            for(int i = lowerbound  + 1; i <= upperbound; i += 1){
                temp = arr[i];
                int j;
                for(j = i - 1; j >= lowerbound && temp < arr[j]; j -= 1){
                    arr[j+1] = arr[j];
                }
                arr[j+1] = temp;
            }
        }
    return;
    }

    //------------partition code copied from the function above, there used to be a function call here. Removed per assignment instruction----------
    if(median == 'y'){//handling -m3 input
        int k = (lowerbound + upperbound) / 2;
        if(arr[lowerbound]>arr[k]){
            temp = arr[lowerbound];
            arr[lowerbound] = arr[k];
            arr[k] = temp;
        }
        if(arr[lowerbound]>arr[upperbound]){
            temp = arr[lowerbound];
            arr[lowerbound] = arr[upperbound];
            arr[upperbound] = temp;
        }
        if(arr[k]>arr[upperbound]){
            temp = arr[k];
            arr[k] = arr[upperbound];
            arr[upperbound] = temp;
        }
    }
    //partition algorithm from slides:
    int i = lowerbound;
    int j = upperbound+1;
    int pivot = arr[lowerbound];
    do{
        do{i += 1;}while(arr[i] < pivot);
        do{j -= 1;}while(arr[j] > pivot);
        if(i<j){
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }else{ break; }

    }while(1);
    temp = arr[lowerbound];
    arr[lowerbound] = arr[j];
    arr[j] = temp;
    temp = j;
    //-----------------------------------------------------------------------
    //calls the recursive halves of quicksort
    quickSort(lowerbound, temp-1, temp-lowerbound);
    quickSort(temp+1, upperbound, upperbound-temp); 
}

//Function where all threads start. Passed an interval, which it then uses in its call to quickSort()
void *runner(void *passed){
    struct interval *passedInterval = passed; //retrieving the interval from the pointer
    quickSort((*passedInterval).lower, (*passedInterval).upper, (*passedInterval).size);

    pthread_exit(0); //exit so main can join
}