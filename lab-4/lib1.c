#include "lib.h"

// Наивный алгоритм подсчёта простых чисел
// Проверить делимость текущего числа на все предыдущие числа
int prime_count(int a, int b) {
    int count = 0;
    for (int num = a; num <= b; num++) {
        if (num < 2) {
            continue;
        }
        int is_prime = 1;
        for (int i = 2; i < num; i++) {
            if (num % i == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) {
            count++;
        }
    }
    return count;
}

// Ряд Лейбница
float pi(int k) {
    float res = 0;
    for (int i = 0; i < k; i++) {
        res += (i & 1 ? -1. : 1.) / (2 * i + 1); 
    }
    return res * 4;
}

