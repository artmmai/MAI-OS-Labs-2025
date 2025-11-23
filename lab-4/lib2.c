#include "lib.h"

// Решето Эратосфена
int prime_count(int a, int b) {
    if (b < 2) { 
        return 0;
    }
    
    // Массив: true = простое, false = составное
    int *is_prime = (int *)calloc(b + 1, sizeof(int));
    
    // Инициализация: все числа >= 2 считаем простыми
    for (int i = 2; i <= b; i++) {
        is_prime[i] = 1;
    }
    
    // Отсеивание составных чисел
    for (int i = 2; i * i <= b; i++) {
        if (is_prime[i]) {
            // Вычёркиваем кратные
            for (int composite = i * i; composite <= b; composite += i) {
                is_prime[composite] = 0;
            }
        }
    }
    
    // Подсчёт простых в [a, b]
    int count = 0;
    int start = (a < 2) ? 2 : a;
    for (int num = start; num <= b; num++) {
        if (is_prime[num]) {
            count++;
        }
    }
    
    free(is_prime);
    return count;
}


// Формула Валлиса
float pi(int k) {
    float res = 1;
    for (int i = 1; i <= k; i++) {
        res *= 4. * i * i / (4 * i * i - 1);
    }
    return res * 2;
}
