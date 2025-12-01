#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct
{
    long num_points;
    long points_in_circle;
    unsigned int seed;
} thread_data_t;

double rand_double(unsigned int *seed, double a, double b) {
    double r = (double)rand_r(seed) / (double)RAND_MAX;
    return a + r * (b - a);
}
;

void *monte_carlo_thread(void *input)
{
    thread_data_t *data = (thread_data_t *)input;
    long count = 0;
    for (long i = 0; i < data->num_points; ++i)
    {
        double x = rand_double(&data->seed, -1.0, 1.0);
        double y = rand_double(&data->seed, -1.0, 1.0);

        if (x * x + y * y <= 1.0)
            ++count;
    }
    data->points_in_circle = count;
    pthread_exit(NULL);
}
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <num_threads> <num_points>\n", argv[0]);
        printf("  num_threads: Số lượng thread\n");
        printf("  num_points: Tổng số điểm để tính Pi\n");
        return 1;
    }

    long num_threads = atoi(argv[1]);
    long total_points = atoi(argv[2]);

    if (num_threads <= 0 || total_points <= 0)
    {
        printf("Lỗi: num_threads và total_points là số dương\n");
        return 1;
    }

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    thread_data_t *thread_data = malloc(num_threads * sizeof(thread_data_t));

    if (threads == NULL || thread_data == NULL)
    {
        printf("Error: Không thể cấp phát bộ nhớ\n");
        free(threads);
        free(thread_data);
        return 1;
    }

    long points_per_thread = total_points / num_threads;
    long remaining_points = total_points % num_threads;

    clock_t start = clock();

    for (long i = 0; i < num_threads; ++i) {
        thread_data[i].num_points = points_per_thread + (i < remaining_points ? 1: 0);
        thread_data[i].points_in_circle = 0;
        thread_data[i].seed =  (unsigned int)time(NULL) ^ (unsigned int)(i * 0x9e3779b9);;

        if(pthread_create(&threads[i], NULL, monte_carlo_thread, &thread_data[i]) != 0) {
            printf("Error: Không thể tạo thread %ld\n", i);
            free(threads);
            free(thread_data);
            return 1;
        }
    }

     long total_in_circle = 0;
    for (int i = 0; i < num_threads; ++i)
    {
        pthread_join(threads[i], NULL);
        total_in_circle += thread_data[i].points_in_circle;
    }

    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    // Tính Pi
    double pi_estimate = 4.0 * total_in_circle / total_points;

    printf("Số thread: %ld\n", num_threads);
    printf("Tổng số điểm: %ld\n", total_points);
    printf("Điểm trong vòng tròn: %ld\n", total_in_circle);
    printf("Ước lượng Pi: %.10f\n", pi_estimate);
    printf("Thời gian thực thi: %.6f giây\n", elapsed_time);

    free(threads);
    free(thread_data);
    return 0;
}