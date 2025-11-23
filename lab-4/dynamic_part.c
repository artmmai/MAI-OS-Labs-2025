#include <stdio.h>
#include <dlfcn.h>

typedef int (*prime_count_t)(int, int);
typedef float (*pi_t)(int);

char *paths[] = {"./libmy1.so", "./libmy2.so"};

int main() {
    int ind = 0;
    void *lib = dlopen(paths[ind], RTLD_LAZY);
    if (!lib) {
        printf("Ошибка загрузки библиотеки\n");
        return 1;
    }
    
    prime_count_t prime_func = dlsym(lib, "prime_count");
    pi_t pi_func = dlsym(lib, "pi");
    
    
    if (!prime_func || !pi_func) {
        printf("Ошибка загрузки функций\n");
        return 1;
    }

    printf("0 - переключить библиотеку\n");
    printf("1 a b - подсчёт простых\n");
    printf("2 k - вычисление π\n");
    printf("-1 - выход\n\n");
    
    int choice;
    scanf("%d", &choice);
    while (choice != -1) {
        if (choice == 0) {
            dlclose(lib);
            ind = !ind; // переключатель
            lib = dlopen(paths[ind], RTLD_LAZY);

            if (!lib) { 
                printf("Ошибка переключения библиотеки\n");
                return 1;
            }

            prime_func = dlsym(lib, "prime_count");
            pi_func = dlsym(lib, "pi");
            
            if (!prime_func || !pi_func) {
                printf("Ошибка загрузки функций\n");
                return 1;
            }

            printf("Библиотека переключена\n");
        }

        if (choice == 1) {
            int a, b;
            scanf("%d %d", &a, &b);
            printf("Результат: %d\n", prime_func(a, b));
        }
        if (choice == 2) {
            int k;
            scanf("%d", &k);
            printf("Результат: %.10f\n", pi_func(k));
        }
        scanf("%d", &choice);
    }
    dlclose(lib);
    return 0;
}
