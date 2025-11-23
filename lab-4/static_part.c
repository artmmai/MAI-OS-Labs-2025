#include <stdio.h>
#include "lib.h"

int main() {
    int choice;
    printf("1 a b - подсчёт простых чисел на [a,b]\n");
    printf("2 k - вычисление π при длине ряда k\n");
    printf("0 - выход\n\n");
    
    scanf("%d", &choice);
    while (choice) {
        if (choice == 1) {
            int a, b;
            scanf("%d %d", &a, &b);
            printf("Результат: %d\n", prime_count(a, b));
        }
        if (choice == 2) {
            int k;
            scanf("%d", &k);
            printf("Результат: %.10f\n", pi(k));
        }
        scanf("%d", &choice);
    }
    return 0;
}
