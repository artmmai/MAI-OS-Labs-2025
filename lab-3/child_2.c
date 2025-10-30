#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_SIZE 4096

int main(int argc, char **argv) {
    if (argc < 5) {
        const char msg[] = "error: not enough arguments\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    const char *SHM_NAME = argv[1];
    const char *SEM_NAME_SERVER = argv[2];
    const char *SEM_NAME_CHILD_1 = argv[3];
    const char *SEM_NAME_CHILD_2 = argv[4];

    int shm = shm_open(SHM_NAME, O_RDWR, 0600);
    if (shm == -1) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    sem_t *sem_child2 = sem_open(SEM_NAME_CHILD_2, O_RDWR);
    sem_t *sem_server = sem_open(SEM_NAME_SERVER, O_RDWR);
    if (sem_child2 == SEM_FAILED || sem_server == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphores\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    bool running = true;
    while (running) {
        sem_wait(sem_child2);

        uint32_t *length = (uint32_t *)shm_buf;
        char *text = shm_buf + sizeof(uint32_t);

        if (*length == UINT32_MAX) {
            // сообщаем серверу, что пора завершаться
            sem_post(sem_server);
            running = false;
            break;
        }

        if (*length > 0) {
            // заменяем пробельные символы на '_'
            for (uint32_t i = 0; i < *length; ++i) {
                if (text[i] == ' ' || text[i] == '\t') {
                    text[i] = '_';
                }
            }
        }

        // передаём управление серверу
        sem_post(sem_server);
    }

    sem_close(sem_child2);
    sem_close(sem_server);
    munmap(shm_buf, SHM_SIZE);
    close(shm);
    return 0;
}
