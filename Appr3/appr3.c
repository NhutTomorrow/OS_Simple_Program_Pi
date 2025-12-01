// #include <stdio.h>
// #include <stdlib.h>
// #include <pthread.h>
// #include <time.h>
// #include <stdint.h>   // dùng uint64_t, uint32_t

// // Biến toàn cục
// long long nPoints = 0;         
// int nThreads = 0;              
// long long global_count = 0;     

// pthread_mutex_t lock;           


// static inline double rand01_from_index(long long i, uint32_t base_seed) {
    
//     uint64_t x = ((uint64_t)i) ^ (uint64_t)base_seed;

//     x ^= x >> 12;
//     x ^= x << 25;
//     x ^= x >> 27;

//     x *= 2685821657736338717ULL;

//     // Lấy 53 bit để map sang double [0,1)
//     uint64_t mantissa = (x >> 11) & ((1ULL << 53) - 1);
//     return (double)mantissa / (double)(1ULL << 53);
// }

// // Thread function
// void *worker(void *arg) {
//     long long thread_id = (long long)arg;

//     // Chia đoạn i cho từng thread
//     long long points_per_thread = nPoints / nThreads;
//     long long start = thread_id * points_per_thread;
//     long long end   = (thread_id == nThreads - 1) ? nPoints : (start + points_per_thread);

//     long long local_count = 0;

//     for (long long i = start; i < end; i++) {
//         // x, y phụ thuộc i, KHÔNG phụ thuộc thread
//         double rx = rand01_from_index(i, 76480680);
//         double ry = rand01_from_index(i, 123456u);

//         double x = rx * 2.0 - 1.0;  // [-1,1]
//         double y = ry * 2.0 - 1.0;  // [-1,1]

//         if (x * x + y * y <= 1.0) {
//             local_count++;   // đếm cục bộ
//         }
//     }

//     // Cập nhật biến toàn cục (shared) – 1 lần lock cho mỗi thread
//     pthread_mutex_lock(&lock);
//     global_count += local_count;
//     pthread_mutex_unlock(&lock);

//     return NULL;
// }

// int main(int argc, char *argv[]) {
//     if (argc != 3) {
//         printf("Usage: %s <nPoints> <nThreads>\n", argv[0]);
//         return 1;
//     }

//     nPoints = atoll(argv[1]);
//     nThreads = atoi(argv[2]);

//     if (nPoints <= 0 || nThreads <=0) {
//         printf("nPoints và nThreads phải > 0\n");
//         return 1;
//     }

//     pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * nThreads);
//     if (threads == NULL) {
//         perror("malloc");
//         return 1;
//     }

//     // Khởi tạo mutex
//     pthread_mutex_init(&lock, NULL);

//     struct timespec start, end;
//     clock_gettime(CLOCK_MONOTONIC, &start);

//     // Tạo thread
//     for (long long i = 0; i < nThreads; i++) {
//         if (pthread_create(&threads[i], NULL, worker, (void *)i) != 0) {
//             perror("pthread_create");
//             return 1;
//         }
//     }

//     // Chờ các thread kết thúc
//     for (int i = 0; i < nThreads; i++) {
//         pthread_join(threads[i], NULL);
//     }

//     clock_gettime(CLOCK_MONOTONIC, &end);

//     // Tính pi
//     double pi_estimate = 4.0 * (double)global_count / (double)nPoints;

//     double elapsed = (end.tv_sec - start.tv_sec) +
//                      (end.tv_nsec - start.tv_nsec) / 1e9;

//     printf("Total points       : %lld\n", nPoints);
//     printf("Threads            : %d\n", nThreads);
//     printf("Points in circle   : %lld\n", global_count);
//     printf("Estimated pi       : %.10f\n", pi_estimate);
//     printf("Execution time (s) : %.6f\n", elapsed);

//     // Hủy mutex + free
//     pthread_mutex_destroy(&lock);
//     free(threads);

//     return 0;
// }
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
unsigned int base_seed = 123456;
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

double shared_variable_worker(long long total_points, int num_threads, unsigned int seed) {
    nPoints = total_points;
    nThreads = num_threads;
    base_seed = seed;
    global_count = 0;  // Reset counter
    
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * nThreads);
    if (threads == NULL) {
        fprintf(stderr, "Error: Cannot allocate memory\n");
        return -1.0;
    }

    pthread_mutex_init(&lock, NULL);

    // Tạo threads
    for (long long i = 0; i < nThreads; i++) {
        if (pthread_create(&threads[i], NULL, worker, (void *)i) != 0) {
            perror("pthread_create");
            free(threads);
            pthread_mutex_destroy(&lock);
            return -1.0;
        }
    }

    // Chờ threads hoàn thành
    for (int i = 0; i < nThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Tính Pi
    double pi_estimate = 4.0 * (double)global_count / (double)nPoints;

    pthread_mutex_destroy(&lock);
    free(threads);

    return pi_estimate;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <nPoints> <nThreads>\n", argv[0]);
        return 1;
    }

    long long points = atoll(argv[1]);
    int threads = atoi(argv[2]);

    if (points <= 0 || threads <= 0) {
        printf("nPoints và nThreads phải > 0\n");
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Gọi worker function
    double pi_estimate = shared_variable_worker(points, threads, 123456);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (pi_estimate < 0) {
        printf("Error in computation\n");
        return 1;
    }

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Total points       : %lld\n", points);
    printf("Threads            : %d\n", threads);
    printf("Points in circle   : %lld\n", global_count);
    printf("Estimated pi       : %.10f\n", pi_estimate);
    printf("Execution time (s) : %.6f\n", elapsed);

    return 0;
}