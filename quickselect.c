#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int partition(uint32_t* arr, int low, int high) {
    int pivot_idx = low + rand() % (high - low + 1);
    uint32_t pivot = arr[pivot_idx];
    int i = low - 1, j = high + 1;
    while (1) {
        do i++; while (arr[i] < pivot);
        do j--; while (arr[j] > pivot);
        if (i >= j) return j;
        uint32_t temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

uint32_t quickselect_memmap(uint32_t* arr, int low, int high, int k) {
    while (low < high) {
        int p = partition(arr, low, high);
        if (k <= p)
            high = p;
        else
            low = p + 1;
    }
    return arr[low];
}

void generate_and_save_data(int n, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("File opening failed");
        exit(1);
    }
    uint32_t* data = (uint32_t*)malloc(n * sizeof(uint32_t));
    for (int i = 0; i < n; i++)
        data[i] = rand();
    fwrite(data, sizeof(uint32_t), n, file);
    free(data);
    fclose(file);
}

void quicksort(uint32_t* arr, int low, int high) {
    if (low < high) {
        int p = partition(arr, low, high);
        quicksort(arr, low, p);
        quicksort(arr, p + 1, high);
    }
}

void benchmark_task1(int* n_values, int num_n, int iterations) {
    for (int i = 0; i < num_n; i++) {
        int n = n_values[i];
        char filename[256];
        sprintf(filename, "data_%d.bin", n);

        struct stat st;
        if (stat(filename, &st) != 0)
            generate_and_save_data(n, filename);
        int adjusted_iter = iterations;

        double qs_total = 0;
        for (int j = 0; j < adjusted_iter; j++) {
            int fd = open(filename, O_RDWR);
            uint32_t* arr = mmap(NULL, n * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            int k = rand() % n;
            clock_t start = clock();
            quickselect_memmap(arr, 0, n - 1, k);
            qs_total += (double)(clock() - start) / CLOCKS_PER_SEC;
            munmap(arr, n * sizeof(uint32_t));
            close(fd);
        }
        double avg_qs = qs_total / adjusted_iter;

        int fd = open(filename, O_RDWR);
        uint32_t* arr = mmap(NULL, n * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        clock_t start = clock();
        quicksort(arr, 0, n - 1);
        double qsort_time = (double)(clock() - start) / CLOCKS_PER_SEC;

        munmap(arr, n * sizeof(uint32_t));
        close(fd);

        printf("n=%d: QuickSelect %.4fs, QuickSort %.4fs\n", n, avg_qs, qsort_time);
    }
}

int main() {
    srand(time(NULL));
    printf("Task 1 Benchmarks:\n");
    int task1_n[] = {10000000, 20000000, 40000000, 80000000, 160000000};
    benchmark_task1(task1_n, 5, 100);
    return 0;
}