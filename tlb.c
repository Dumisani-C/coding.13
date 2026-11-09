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
