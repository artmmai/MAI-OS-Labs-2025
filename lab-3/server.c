#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <wait.h>
#include <semaphore.h>

#define SHM_SIZE 4096

char SHM_NAME[1024] = "shm-name";
char SEM_NAME_SERVER[1024] = "sem-server";
char SEM_NAME_CHILD_1[1024] = "sem-child-1";
char SEM_NAME_CHILD_2[1024] = "sem-child-2";

int main(void) {
    char unique_suffix[64];
    snprintf(unique_suffix, sizeof(unique_suffix), "%d", getpid());

    snprintf(SHM_NAME, sizeof(SHM_NAME), "shm_%s", unique_suffix);
    snprintf(SEM_NAME_SERVER, sizeof(SEM_NAME_SERVER), "sem_server_%s", unique_suffix);
    snprintf(SEM_NAME_CHILD_1, sizeof(SEM_NAME_CHILD_1), "sem_child1_%s", unique_suffix);
    snprintf(SEM_NAME_CHILD_2, sizeof(SEM_NAME_CHILD_2), "sem_child2_%s", unique_suffix);

    int shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600);
    
    // O_CREAT - создать, если не существует
    // O_TRUNC - если существует, то очистить
    
    if (shm == -1) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    // резервируем размер shared_memory перед mmap()
    if (ftruncate(shm, SHM_SIZE) == -1) {
        const char msg[] = "error: failed to resize SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    // отображаем память в адресное пространство процесса
    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }


    // Делаем sem_server открытым, чтобы parent не блокировался на старте
    // и не завис до отправки строки
    sem_t *sem_server = sem_open(SEM_NAME_SERVER, O_RDWR | O_CREAT | O_TRUNC, 0600, 1);
    if (sem_server == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore sem_server\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    sem_t *sem_child_1 = sem_open(SEM_NAME_CHILD_1, O_RDWR | O_CREAT | O_TRUNC, 0600, 0);
    if (sem_child_1 == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore sem_child_1\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    sem_t *sem_child_2 = sem_open(SEM_NAME_CHILD_2, O_RDWR | O_CREAT | O_TRUNC, 0600, 0);
    if (sem_child_2 == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore sem_child_2\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    // создаём child_1
    pid_t child1 = fork();
    if (child1 == 0) {
        char *args[] = {"child_1", SHM_NAME, SEM_NAME_SERVER, SEM_NAME_CHILD_1, SEM_NAME_CHILD_2, NULL};
        execv("./child_1", args);
        const char msg[] = "error: failed to exec child_1\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    } else if (child1 == -1) {
        const char msg[] = "error: failed to fork child1\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    // создаём child_2
    pid_t child2 = fork();
    if (child2 == 0) {
        char *args[] = {"child_2", SHM_NAME, SEM_NAME_SERVER, SEM_NAME_CHILD_1, SEM_NAME_CHILD_2, NULL};
        execv("./child_2", args);
        const char msg[] = "error: failed to exec child_2\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    } else if (child2 == -1) {
        const char msg[] = "error: failed to fork child2\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    bool running = true;
    while (running) {
        sem_wait(sem_server);

        uint32_t *length = (uint32_t*)shm_buf;
        char *text = shm_buf + sizeof(uint32_t);

        if (*length == UINT32_MAX) {
            running = false;
            sem_post(sem_server);
            break;
        }

        if (*length > 0) {
            // есть результат от child2 — выводим
            const char out[] = "Result: ";
            write(STDOUT_FILENO, out, sizeof(out) - 1);
            write(STDOUT_FILENO, text, *length);
            write(STDOUT_FILENO, "\n", 1);

            // сбрасываем длину для следующего ввода
            *length = 0;
            sem_post(sem_server); // освобождаем серверный семафор для ввода
            continue;
        }

        const char prom[] = "Enter text (Ctrl+Z to exit): ";
        write(STDOUT_FILENO, prom, sizeof(prom) - 1);
        char buf[SHM_SIZE - sizeof(uint32_t)];
        ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf));
        if (bytes == -1) {
            const char err[] = "error: failed to read stdin\n";
            write(STDERR_FILENO, err, sizeof(err));
            _exit(EXIT_FAILURE);
        }

        if (bytes > 0) {
            *length = (uint32_t)bytes;
            memcpy(text, buf, bytes);
            sem_post(sem_child_1); // запускаем child1 (child1 -> child2 -> server)
        } else {
            *length = UINT32_MAX;
            sem_post(sem_child_1);
            running = false;
        }
    }

    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    sem_unlink(SEM_NAME_SERVER);
    sem_unlink(SEM_NAME_CHILD_1);
    sem_unlink(SEM_NAME_CHILD_2);

    sem_close(sem_server);
    sem_close(sem_child_1);
    sem_close(sem_child_2);

    munmap(shm_buf, SHM_SIZE);
    shm_unlink(SHM_NAME);
    close(shm);

    return 0;
}
