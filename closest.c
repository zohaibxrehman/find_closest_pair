#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "point.h"
#include "utilities_closest.h"
#include "serial_closest.h"
#include "parallel_closest.h"


void print_usage() {
    fprintf(stderr, "Usage: closest -f filename -d pdepth\n\n");
    fprintf(stderr, "    -d Maximum process tree depth\n");
    fprintf(stderr, "    -f File that contains the input points\n");

    exit(1);
}

int main(int argc, char **argv) {
    int n = -1;
    long pdepth = -1;
    char *filename = NULL;
    int pcount = 0;

    // Parsing the command line arguments with d and f flag
    int opt;
    int d_flag = 0;
    int f_flag = 0;
    while ((opt = getopt(argc, argv, "d:f:")) != -1) {
        switch (opt) {
            case 'f':
                f_flag = 1;
                filename = optarg;
                break;
            case 'd':
                d_flag = 1;
                pdepth = strtol(optarg, NULL, 10);
                break;
            default: /* '?' */
                print_usage();
                exit(1);
        }
    }
    
    // printing usage if user doesn't either
    // d or f flag or both
    if (!f_flag || !d_flag) {
        print_usage();
    }

    // Read the points
    n = total_points(filename);
    struct Point points_arr[n];
    read_points(filename, points_arr);

    // Sort the points
    qsort(points_arr, n, sizeof(struct Point), compare_x);

    // Calculate the result using the parallel algorithm.
    double result_p = closest_parallel(points_arr, n, pdepth, &pcount);
    printf("The smallest distance: is %.2f (total worker processes: %d)\n", result_p, pcount);

    exit(0);
}
