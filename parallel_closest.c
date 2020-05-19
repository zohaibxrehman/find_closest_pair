#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"


/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
    if(n < 4 || pdmax == 0) {
        return closest_serial(p, n);
    }

    // Pointer p will be reused as left child pointer
    // right will be used for the right child pointer 
    int mid = n / 2;
    struct Point mid_point = p[mid];
    struct Point *right = p + mid;

    int fd1[2];
    int fd2[2];
    int result_1, result_2;
    if ((pipe(fd1)) == -1) {
        exit(1);
    }

    // left child fork at result_1 = 0
    result_1 = fork();
    if(result_1 < 0) {
        perror("pipe");
        exit(1);
    } else if (result_1 == 0) {
        if(close(fd1[0]) == -1) {
            perror("pipe");
            exit(1);
        }
        double closest_1 = closest_parallel(p, mid, pdmax - 1, pcount);
        if(write(fd1[1], &closest_1, sizeof(closest_1)) == -1){
            perror("write");
            exit(1);
        }

        if(close(fd1[1]) == -1){
            perror("close");
            exit(1);
        }
        exit(*pcount);
    } else if (result_1 > 0) {
        if(close(fd1[1]) == -1) {
            perror("close");
            exit(1);
        }
    }
    
    // right child fork at result_2 = 0
    if(pipe(fd2) == -1){
        perror("pipe");
        exit(1);
    } 
    result_2 = fork();
    if(result_2 < 0){
        perror("fork");
        exit(1);
    }
    else if (result_2 == 0) {
        if(close(fd2[0]) == -1) {
            perror("pipe");
            exit(1);
        }
        double closest_2 = closest_parallel(right, n - mid, pdmax - 1, pcount);
        if(write(fd2[1], &closest_2, sizeof(closest_2)) == -1){
            perror("write");
            exit(1);
        }
        if(close(fd2[1]) == -1) {
            perror("close");
            exit(1);
        }
        exit(*pcount);
    } else if(result_2 > 0){
        if(close(fd2[1]) == -1) {
            perror("close");
            exit(1);
        }
    }

    // adding up # of worker processes
    int status;
    *pcount += 2;
    for(int i = 0; i < 2; i++) {
        if(wait(&status) == -1) {
            perror("wait");
            exit(1);
        }
        if(WIFEXITED(status)){
            *pcount += WEXITSTATUS(status);
        }
    }

    // reading to get the min dist from children
    double left_dist;
    if (read(fd1[0], &left_dist, sizeof(left_dist)) == -1){
        perror("read");
        exit(1);
    } 
    if (close(fd1[0]) == -1) {
        perror("close");
        exit(1);
    } 

    double right_dist;
    if (read(fd2[0], &right_dist, sizeof(right_dist)) == -1){
        perror("read");
        exit(1);
    }
    if (close(fd2[0]) == -1) {
        perror("close");
        exit(1);
    }

    double d = min(left_dist, right_dist);

    // Build an array strip[] that contains points close (closer than d) to the line passing through the middle point.
    struct Point *strip = malloc(sizeof(struct Point) * n);
    if (strip == NULL) {
        perror("malloc");
        exit(1);
    }

    int j = 0;
    for (int i = 0; i < n; i++) {
        if (abs(p[i].x - mid_point.x) < d) {
            strip[j] = p[i], j++;
        }
    }

    // Find the closest points in strip.  Return the minimum of d and closest distance in strip[].
    double new_min = min(d, strip_closest(strip, j, d));
    free(strip);

    return new_min;
}

