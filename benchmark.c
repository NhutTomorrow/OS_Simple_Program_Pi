#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// ==================== Global Configuration ====================
long long global_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// ==================== Utility ====================
static inline double rand_double(unsigned int *seed, double a, double b) {
    return a + ((double)rand_r(seed) / RAND_MAX) * (b - a);
}

// ==================== Single Thread ====================
double worker_single_thread(long long npoints, unsigned int seed) {
    long long inside = 0;
    for (long long i = 0; i < npoints; i++) {
        unsigned int local_seed = seed + i;
        double x = rand_double(&local_seed, -1.0, 1.0);
        double y = rand_double(&local_seed, -1.0, 1.0);
        if (x * x + y * y <= 1.0) inside++;
    }
    return 4.0 * inside / npoints;
}

// ==================== Multi-threaded (Local Count) ====================
typedef struct {
    long long start_idx;
    long long num_points;
    unsigned int base_seed;
    long long points_in_circle;
} thread_data_t;

void *monte_carlo_worker(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    long long count = 0;
    
    for (long long i = 0; i < data->num_points; i++) {
        unsigned int seed = data->base_seed + data->start_idx + i;
        double x = rand_double(&seed, -1.0, 1.0);
        double y = rand_double(&seed, -1.0, 1.0);
        if (x * x + y * y <= 1.0) count++;
    }
    
    data->points_in_circle = count;
    return NULL;
}

double worker_multi_thread(long long total_points, int num_threads, unsigned int base_seed) {
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];
    
    long long points_per_thread = total_points / num_threads;
    long long remaining = total_points % num_threads;
    long long start_idx = 0;
    
    for (int i = 0; i < num_threads; i++) {
        long long pts = points_per_thread + (i < remaining ? 1 : 0);
        thread_data[i] = (thread_data_t){start_idx, pts, base_seed, 0};
        pthread_create(&threads[i], NULL, monte_carlo_worker, &thread_data[i]);
        start_idx += pts;
    }
    
    long long total_in_circle = 0;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_in_circle += thread_data[i].points_in_circle;
    }
    
    return 4.0 * total_in_circle / total_points;
}

// ==================== Shared Variable (with Mutex) ====================
typedef struct {
    long long start_idx;
    long long num_points;
    unsigned int base_seed;
} shared_data_t;

void *shared_worker(void *arg) {
    shared_data_t *data = (shared_data_t *)arg;
    long long local_count = 0;
    
    // Accumulate locally first to reduce mutex contention
    for (long long i = 0; i < data->num_points; i++) {
        unsigned int seed = data->base_seed + data->start_idx + i;
        double x = rand_double(&seed, -1.0, 1.0);
        double y = rand_double(&seed, -1.0, 1.0);
        if (x * x + y * y <= 1.0) local_count++;
    }
    
    // Update global count once
    pthread_mutex_lock(&lock);
    global_count += local_count;
    pthread_mutex_unlock(&lock);
    
    return NULL;
}

double worker_shared_variable(long long total_points, int num_threads, unsigned int base_seed) {
    pthread_t threads[num_threads];
    shared_data_t thread_data[num_threads];
    
    global_count = 0;
    
    long long points_per_thread = total_points / num_threads;
    long long remaining = total_points % num_threads;
    long long start_idx = 0;
    
    for (int i = 0; i < num_threads; i++) {
        long long pts = points_per_thread + (i < remaining ? 1 : 0);
        thread_data[i] = (shared_data_t){start_idx, pts, base_seed};
        pthread_create(&threads[i], NULL, shared_worker, &thread_data[i]);
        start_idx += pts;
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 4.0 * global_count / total_points;
}

// ==================== Benchmark Runner ====================
void run_benchmark(long long npoints, unsigned int seed) {
    struct timespec t0, t1;

    printf("════════════════════════════════════════════════════════════════\n");
    printf("Points: %lld | Seed: %u\n", npoints, seed);
    printf("════════════════════════════════════════════════════════════════\n");

    // ==================== BASELINE ====================
    printf("\n[BASELINE] Single Thread\n");
    clock_gettime(CLOCK_MONOTONIC, &t0);
    double pi1 = worker_single_thread(npoints, seed);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double baseline = (t1.tv_sec - t0.tv_sec) +
                      (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("Pi = %.10f, Time = %.6f s\n", pi1, baseline);

    // ==================== APPROACH 2 ====================
    printf("\n════════════════════════════════════════════════════════════════\n");
    printf("APPROACH 2: MULTI-THREADED\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("%-8s | %-14s | %-12s | %-10s\n",
           "Threads", "Pi", "Time (s)", "Speedup");
    printf("────────────────────────────────────────────────────────────────\n");

    double time_appr2[101] = {0};   // lưu A2(N)

    for (int nt = 2; nt <= 100; nt++) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        double pi2 = worker_multi_thread(npoints, nt, seed);
        clock_gettime(CLOCK_MONOTONIC, &t1);

        double elapsed = (t1.tv_sec - t0.tv_sec) +
                         (t1.tv_nsec - t0.tv_nsec) / 1e9;

        time_appr2[nt] = elapsed;

        printf("%-8d | %14.10f | %12.6f | %10.2fx\n",
               nt, pi2, elapsed, baseline / elapsed);
    }

    // ==================== APPROACH 3 ====================
    printf("\n════════════════════════════════════════════════════════════════\n");
    printf("APPROACH 3: SHARED VARIABLE\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("%-8s | %-14s | %-12s | %-10s\n",
           "Threads", "Pi", "Time (s)", "Speedup(A2->A3)");
    printf("────────────────────────────────────────────────────────────────\n");

    for (int nt = 2; nt <= 100; nt++) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        double pi3 = worker_shared_variable(npoints, nt, seed);
        clock_gettime(CLOCK_MONOTONIC, &t1);

        double elapsed = (t1.tv_sec - t0.tv_sec) +
                         (t1.tv_nsec - t0.tv_nsec) / 1e9;

        double speedup = time_appr2[nt] / elapsed;

        printf("%-8d | %14.10f | %12.6f | %10.2fx\n",
               nt, pi3, elapsed, speedup);
    }

    printf("════════════════════════════════════════════════════════════════\n");
}

// ==================== Main ====================
int main(int argc, char *argv[]) {
    long long npoints = 100000000;  // 100M points
    unsigned int seed = 123456;
    
    if (argc > 1) npoints = atoll(argv[1]);
    if (argc > 2) seed = atoi(argv[2]);
    
    if (npoints <= 0) {
        fprintf(stderr, "Usage: %s [npoints] [seed]\n", argv[0]);
        return 1;
    }
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  MONTE CARLO PI - THREAD SCALING BENCHMARK                ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    run_benchmark(npoints, seed);
    
    printf("\nCompleted!\n\n");
    return 0;
}