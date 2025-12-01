
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Biến toàn cục
long long nPoints = 0;         
int nThreads = 0;              
long long global_count = 0;     
long long start_idx = 0;
long long end_idx = 0;
unsigned int base_seed = 0;
pthread_mutex_t lock;           


double rand_double(unsigned int *seed, double a, double b) {
    double r = (double)rand_r(seed) / (double)RAND_MAX;
    return a + r * (b - a);
}


void *worker(void *arg) {
    long long thread_id = (long long)arg;

    
    long long points_per_thread = nPoints / nThreads;

    long long start_idx = thread_id * points_per_thread;
    long long end_idx = start_idx + points_per_thread;

    if (thread_id == nThreads - 1) {
        end_idx = nPoints;
    }
 
    // unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(thread_id * 0x9e3779b9);
    

   
    for (long long i = start_idx; i < end_idx; i++) {
        unsigned int seed = base_seed + (unsigned int)i;
        double x = rand_double(&seed, -1.0, 1.0);
        double y = rand_double(&seed, -1.0, 1.0);

        if (x * x + y * y <= 1.0) {
           
            pthread_mutex_lock(&lock);
            global_count++;
            pthread_mutex_unlock(&lock);
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <nPoints> <nThreads>\n", argv[0]);
        return 1;
    }

    nPoints = atoll(argv[1]);
    nThreads = atoi(argv[2]);

    if (nPoints <= 0 || nThreads <=0) {
        printf("nPoints và nThreads phải > 0\n");
        return 1;
    }

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * nThreads);
    if (threads == NULL) {
        perror("malloc");
        return 1;
    }

    // Khởi tạo mutex
    pthread_mutex_init(&lock, NULL);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Tạo thread
    for (long long i = 0; i < nThreads; i++) {
        if (pthread_create(&threads[i], NULL, worker, (void *)i) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    // Chờ các thread kết thúc
    for (int i = 0; i < nThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Tính pi
    double pi_estimate = 4.0 * (double)global_count / (double)nPoints;

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Total points       : %lld\n", nPoints);
    printf("Threads            : %d\n", nThreads);
    printf("Points in circle   : %lld\n", global_count);
    printf("Estimated pi       : %.10f\n", pi_estimate);
    printf("Execution time (s) : %.6f\n", elapsed);

    // Hủy mutex + free
    pthread_mutex_destroy(&lock);
    free(threads);

    return 0;
}