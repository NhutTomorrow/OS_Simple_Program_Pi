#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

long int N = 1000000000;
unsigned int SEED = 123456;
double worker_single_thread( long int npoints, unsigned int seed);

double rand_double(unsigned int *seed, double a, double b) {
    double r = (double)rand_r(seed) / (double)RAND_MAX;
    return a + r * (b - a);
}


int main(int argc, char *argv[]) {

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [number_of_points]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        char *endptr = NULL;
        unsigned long long val = strtoull(argv[1], &endptr, 10);
        if (endptr == NULL || *endptr != '\0' || val == 0) {
            fprintf(stderr, "Invalid number: %s\n", argv[1]);
            return 1;
        }
        N = val;
    }
    
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    double pi = worker_single_thread(N,SEED);
    clock_gettime(CLOCK_MONOTONIC, &t1); // Lấy thời gian kết thúc
    double elapsed_sec = (t1.tv_sec - t0.tv_sec) +
                     (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("Elapsed: %.6f s\n", elapsed_sec);
    
    printf("Pi ≈ %.10f\n", pi);
    return 0;
}

double worker_single_thread(long int npoints, unsigned int seed){
    long inside = 0L;
    for (long int i = 0; i < npoints; i++) {
        unsigned int seed = SEED + i; // Reset seed cho mỗi điểm
        double x = rand_double(&seed,-1.0,1.0);
        double y = rand_double(&seed,-1.0,1.0);
        if (x * x + y * y <= 1.0) ++inside;
    }
    double pi = 4.0 * inside / npoints;
    return pi;
}