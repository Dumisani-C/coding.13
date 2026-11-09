/*
 * Chapter 19 Homework (Measurement) - Answers
 * Dumisani Chikomo, CSC 425
 *
 * 1. Timer precision:
 *    gettimeofday() only gives microseconds, and clock_gettime() can go down
 *    to nanoseconds, but a single page access only takes a few nanoseconds.
 *    That is way smaller than what one timer reading can reliably catch, so
 *    timing one access by itself would just give 0 or noise. To time it
 *    properly the whole loop has to run for at least a few milliseconds, so I
 *    repeat the page accesses many times (10,000+ trials) and divide the total
 *    time by (pages * trials) to get the average per access. I used
 *    clock_gettime(CLOCK_MONOTONIC) since it is more precise.
 *
 * 2. The program:
 *    tlb.c takes the number of pages and the number of trials as arguments.
 *    It allocates an array that spans that many pages, then touches one int
 *    per page (jumping PAGESIZE / sizeof(int) each time), repeats that for
 *    every trial, and prints the average time per access in nanoseconds.
 *    My runs: 1 page = ~2.5 ns, 64 pages = ~6.9 ns, 4096 pages = ~5.5 ns,
 *    so the cost jumps once the pages no longer fit in the first-level TLB.
 *
 * 5. Compiler optimization:
 *    The compiler could see that nothing reads a[] after the loop and just
 *    delete the loop. To stop that I compile with plain gcc (no -O flag),
 *    which means -O0 and no optimizations. Other options are declaring the
 *    array volatile or using the results afterwards (like printing a sum) so
 *    the compiler has to keep the loop.
 *
 * 6. Multiple CPUs:
 *    Each CPU has its own TLB, so if the scheduler moves the program to
 *    another CPU in the middle of the test, that CPU's TLB is cold and I get
 *    a bunch of extra misses that make the numbers jump around. I pin the
 *    program to CPU 0 with sched_setaffinity() at the start so it stays on
 *    one CPU the whole time.
 *
 * 7. Initialization:
 *    Yes, it would affect the timing. The first time a page is touched the OS
 *    has to page fault and zero it out (demand zeroing), which is way more
 *    expensive than a TLB miss and would make the first trial look really
 *    slow. To fix that I touch every page once before starting the timer, so
 *    only the normal accesses get measured.
 */

// tlb.c - measures the average cost of accessing one page,
// based on the TLB homework in OSTEP Chapter 19.
//
// Usage: ./tlb <number_of_pages> <number_of_trials>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <num_pages> <num_trials>\n", argv[0]);
        return 1;
    }

    int numPages  = atoi(argv[1]);
    int numTrials = atoi(argv[2]);
    if (numPages <= 0 || numTrials <= 0) {
        fprintf(stderr, "num_pages and num_trials must be positive\n");
        return 1;
    }

    // Keep the program on one CPU so it doesn't move between cores
    // (each core has its own TLB).
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);

    long pageSize = sysconf(_SC_PAGESIZE);   // usually 4096 bytes
    int jump = pageSize / sizeof(int);       // ints per page

    int *a = malloc((size_t)numPages * pageSize);
    if (a == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    // Touch every page once first so the timing doesn't include
    // the OS setting up the pages the first time.
    for (long i = 0; i < (long)numPages * jump; i += jump)
        a[i] = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Access one int on each page, numTrials times.
    for (int t = 0; t < numTrials; t++) {
        for (long i = 0; i < (long)numPages * jump; i += jump)
            a[i] += 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double totalNs = (end.tv_sec - start.tv_sec) * 1e9
                   + (end.tv_nsec - start.tv_nsec);
    double perAccess = totalNs / ((double)numPages * numTrials);

    printf("pages: %d  trials: %d  avg time per access: %.2f ns\n",
           numPages, numTrials, perAccess);

    free(a);
    return 0;
}
