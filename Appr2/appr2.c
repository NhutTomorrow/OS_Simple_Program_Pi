#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct
{
    long start_idx;
    long num_points;
    long points_in_circle;
    unsigned int base_seed;
} thread_data_t;

// ==================== Utility Functions ====================
double rand_double(unsigned int *seed, double a, double b)
{
    double r = (double)rand_r(seed) / (double)RAND_MAX;
    return a + r * (b - a);
}

// ==================== Worker Thread Function ====================
void *monte_carlo_worker(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    long count = 0;
    for (long idx = data->start_idx; idx < data->start_idx + data->num_points; idx++)
    {
        unsigned int seed = data->base_seed + idx; // seed dựa trên chỉ số toàn cục
        double x = rand_double(&seed, -1.0, 1.0);
        double y = rand_double(&seed, -1.0, 1.0);

        if (x * x + y * y <= 1.0)
            count++;
    }
    data->points_in_circle = count;
    return NULL;
}

// ==================== Main Multi-thread Worker Function ====================
double worker_multi_thread(long total_points, long num_threads, unsigned int base_seed) {
    // Cấp phát bộ nhớ cho threads và thread data
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    thread_data_t *thread_data = malloc(num_threads * sizeof(thread_data_t));
    
    if (!threads || !thread_data) {
        fprintf(stderr, "Error: Không thể cấp phát bộ nhớ\n");
        free(threads);
        free(thread_data);
        return -1.0;
    }

    // Phân phối điểm cho các threads
    long points_per_thread = total_points / num_threads;
    long remaining_points = total_points % num_threads;
    
    long start_idx = 0;
    
    // Tạo và khởi chạy các threads
    for (long i = 0; i < num_threads; i++) {
        long pts = points_per_thread + (i < remaining_points ? 1 : 0);
        
        thread_data[i].start_idx = start_idx;
        thread_data[i].num_points = pts;
        thread_data[i].points_in_circle = 0;
        thread_data[i].base_seed = base_seed;
        
        pthread_create(&threads[i], NULL, monte_carlo_worker, &thread_data[i]);
        start_idx += pts;
    }

    // Đợi tất cả threads hoàn thành và tổng hợp kết quả
    long total_in_circle = 0;
    for (long i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_in_circle += thread_data[i].points_in_circle;
    }

    // Giải phóng bộ nhớ
    free(threads);
    free(thread_data);

    // Tính và trả về giá trị Pi
    double pi = 4.0 * total_in_circle / total_points;
    return pi;
}

// ==================== Main Function (Demo) ====================
int main() {
    long total_points = 1000000000;  // 1 tỷ điểm
    long num_threads = 8;             // 8 threads
    unsigned int base_seed = 123456;  // seed cố định

    printf("Tính Pi bằng phương pháp Monte Carlo (Multi-threaded)\n");
    printf("Tổng số điểm: %ld\n", total_points);
    printf("Số threads: %ld\n", num_threads);
    printf("----------------------------------------\n");

    // Gọi worker function với 3 tham số
    double pi = worker_multi_thread(total_points, num_threads, base_seed);
    
    if (pi < 0) {
        printf("Lỗi trong quá trình tính toán\n");
        return 1;
    }

    printf("Giá trị Pi: %.10f\n", pi);
    printf("Sai số: %.10f\n", pi - 3.141592653589793);

    return 0;
}