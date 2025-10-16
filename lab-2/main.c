#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

typedef struct {
    double re, im;
} Complex;

typedef struct {
    Complex **A;
    Complex **B;
    Complex **C;
    int n;
    int start_row;
    int end_row;
} ThreadArgs;

static Complex add_complex(Complex a, Complex b) {
    Complex res;
    res.re = a.re + b.re;
    res.im = a.im + b.im;
    return res;
}

static Complex mul_complex(Complex a, Complex b) {
    Complex res;
    res.re = a.re * b.re - a.im * b.im;
    res.im = a.re * b.im + a.im * b.re;
    return res;
}

// Многопоточное умножение части матрицы
static void *multiply_part(void *_args) {
    ThreadArgs *args = (ThreadArgs*)_args;
    for (int i = args->start_row; i < args->end_row; i++) {
        for (int j = 0; j < args->n; j++) {
            Complex sum = {0, 0};
            for (int k = 0; k < args->n; k++)
                sum = add_complex(sum, mul_complex(args->A[i][k], args->B[k][j]));
            args->C[i][j] = sum;
        }
    }
    return NULL;
}

// Вывод матрицы
static void print_matrix(Complex **Matrix, int n, const char *name) {
    printf("\nМатрица %s:\n", name);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            printf("(%5.1f + %5.1fi) ", Matrix[i][j].re, Matrix[i][j].im);
        printf("\n");
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Использование: %s -t <число_потоков>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) {
        printf("Ошибка: количество потоков должно быть > 0\n");
        return 1;
    }

    int n;
    printf("Введите порядок квадратных матриц: ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        printf("Ошибка: введите корректное число\n");
        return 1;
    }

    // Выделеление памяти для матриц
    Complex **A = malloc(n * sizeof(Complex*));
    Complex **B = malloc(n * sizeof(Complex*));
    Complex **C = malloc(n * sizeof(Complex*)); // Для результатов параллельного умножения 
    Complex **D = malloc(n * sizeof(Complex*)); // Для результатов последовательного умножения
    for (int i = 0; i < n; i++) {
        A[i] = malloc(n * sizeof(Complex));
        B[i] = malloc(n * sizeof(Complex));
        C[i] = malloc(n * sizeof(Complex));
        D[i] = malloc(n * sizeof(Complex));
    }

    srand((unsigned)time(NULL));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j].re = rand() % 10;
            A[i][j].im = rand() % 10;
            B[i][j].re = rand() % 10;
            B[i][j].im = rand() % 10;
        }

    print_matrix(A, n, "A");
    print_matrix(B, n, "B");
    printf("Количество потоков: %d\n", num_threads);

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(num_threads * sizeof(ThreadArgs));

    int rows_per_thread = n / num_threads;
    struct timespec start, end;

    // Параллельное умножение
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int t = 0; t < num_threads; t++) {
        thread_args[t] = (ThreadArgs){
            .A = A,
            .B = B,
            .C = C,
            .n = n,
            .start_row = t * rows_per_thread,
            .end_row = (t == num_threads - 1) ? n : (t + 1) * rows_per_thread
        };
        pthread_create(&threads[t], NULL, multiply_part, &thread_args[t]);
    }
    for (int t = 0; t < num_threads; t++)
        pthread_join(threads[t], NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_parallel = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    print_matrix(C, n, "Результат умножения (параллельно)");

    // Последовательное умножение
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            Complex sum = {0, 0};
            for (int k = 0; k < n; k++)
                sum = add_complex(sum, mul_complex(A[i][k], B[k][j]));
            D[i][j] = sum;
        }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double seq_time_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    print_matrix(D, n, "Результат умножения (последовательно)");

    double speedup = seq_time_sec / time_parallel;
    double efficiency = speedup / num_threads;

    printf("\nВремя работы (параллельно): %.6f секунд\n", time_parallel);
    printf("Время работы (последовательно): %.6f секунд\n", seq_time_sec);
    printf("Ускорение = %.2fx\n", speedup);
    printf("Эффективность = %.4f\n", efficiency);

    for (int i = 0; i < n; i++) {
        free(A[i]); free(B[i]); free(C[i]); free(D[i]);
    }
    free(A);
    free(B); 
    free(C);
    free(D);
    free(threads); 
    free(thread_args);

    return 0;
}
