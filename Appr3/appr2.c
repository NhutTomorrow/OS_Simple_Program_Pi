#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    long start_idx;
    long num_points;
    long points_in_circle;
    unsigned int base_seed;
} thread_data_t;

double rand_double(unsigned int *seed, double a, double b) {
    double r = (double)rand_r(seed) / (double)RAND_MAX;
    return a + r * (b - a);
}

// Thread Monte Carlo
void* monte_carlo_thread(void *arg) {
    thread_data_t *data = (thread_data_t*)arg;
    long count = 0;
    for (long idx = data->start_idx; idx < data->start_idx + data->num_points; idx++) {
        unsigned int seed = data->base_seed + idx;  // seed dựa trên chỉ số toàn cục
        double x = rand_double(&seed, -1.0, 1.0);
        double y = rand_double(&seed, -1.0, 1.0);
        if (x*x + y*y <= 1.0)
            count++;
    }
    data->points_in_circle = count;
    return NULL;
}

int main() {
    long total_points = 1000000000; // 1 tỷ điểm
    unsigned int base_seed = 123456; // seed cố định để reproducible

    printf("%-8s %-12s %-12s %-12s %-12s\n", "Threads", "Points", "Pi", "Time(s)", "Speedup");

    // Thời gian đơn luồng
    clock_t start_single = clock();
    long count_single = 0;
    for (long idx = 0; idx < total_points; idx++) {
        
        unsigned int seed = 123456;
        double x = rand_double(&seed, -1.0, 1.0);
        double y = rand_double(&seed, -1.0, 1.0);
        if (x*x + y*y <= 1.0) count_single++;
    }
    double pi_single = 4.0 * count_single / total_points;
    clock_t end_single = clock();
    double time_single = (double)(end_single - start_single)/CLOCKS_PER_SEC;

    // Multi-thread từ 2 → 100
    for (long num_threads = 1; num_threads <= 10; num_threads++) {
        pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
        thread_data_t *thread_data = (thread_data_t*)malloc(num_threads * sizeof(thread_data_t));
        if (!threads || !thread_data) {
            printf("Error: Không thể cấp phát bộ nhớ\n");
            free(threads); free(thread_data);
            return 1;
        }

        long points_per_thread = total_points / num_threads;
        long remaining_points = total_points % num_threads;

        long start_idx = 0;
        clock_t start = clock();
        for (long i = 0; i < num_threads; i++) {
            long pts = points_per_thread + (i < remaining_points ? 1 : 0);
            thread_data[i].start_idx = start_idx;
            thread_data[i].num_points = pts;
            thread_data[i].points_in_circle = 0;
            thread_data[i].base_seed = base_seed;
            pthread_create(&threads[i], NULL, monte_carlo_thread, &thread_data[i]);
            start_idx += pts;
        }

        long total_in_circle = 0;
        for (long i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
            total_in_circle += thread_data[i].points_in_circle;
        }
        clock_t end = clock();
        double elapsed = (double)(end - start)/CLOCKS_PER_SEC;
        double pi_multi = 4.0 * total_in_circle / total_points;
        double speedup = elapsed/time_single;

        printf("%-8ld %-12ld %-12.10f %-12.3f %-12.3f\n",
               num_threads, total_points, pi_multi, elapsed, speedup);

        free(threads); free(thread_data);
    }

    return 0;
}