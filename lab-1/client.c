#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/wait.h>


static char CHILD_PROGRAM_NAME[] = "child";

int main(int argc, char* argv[]){
    char prograth[1024];
    {
        ssize_t len = readlink("/proc/self/exe", prograth, sizeof(prograth) -1);
        if (len == -1){
            const char msg[] = "error: failed to read full program path\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        while (prograth[len] != '/')
            --len;
        prograth[len] = '\0';
    }

    // создание каналов
    int pipe1[2]; // parent -> child1
    int pipe2[2]; // child2 -> parent


    // обработали ошибку создания pipe
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1){
        const char msg[] = "error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    // создание child1

    int pipe_child[2]; // pipe child1 -> child2
    if (pipe(pipe_child) == -1){
        const char msg[] = "error: failed to create internal pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    pid_t child1 = fork();

    if (child1 == -1){
        const char msg[] = "error: failed to spawn child1\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    else if (child1 == 0){
        // перенаправляем stdin child1 на pipe1
        dup2(pipe1[0], STDIN_FILENO); 
        dup2(pipe_child[1], STDOUT_FILENO);
        
        // закрываем лишние концы
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(pipe_child[0]);
        close(pipe_child[1]);

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", prograth, CHILD_PROGRAM_NAME);
        char *const args[] = {CHILD_PROGRAM_NAME, "1", NULL};
        execv(path, args);
        const char msg[] = "error: failed to exec child\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    // создание child2
    pid_t child2 = fork();

    if (child2 == -1){
        const char msg[] = "error: failed to spawn child1\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    else if (child2 == 0){
        // перенастраиваем ввод/вывод для второго ребёнка
        dup2(pipe_child[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(pipe_child[0]);
        close(pipe_child[1]);


        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", prograth, CHILD_PROGRAM_NAME);
        char *const args[] = {CHILD_PROGRAM_NAME, "2", NULL};
        execv(path, args);
        const char msg[] = "error: failed to exec child2\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    // parent
    close(pipe1[0]); // читающий конец 
    close(pipe2[1]); // записывающий конец
    close(pipe_child[0]);
    close(pipe_child[1]); // pipe для детей
     

    char buf[4096];
    ssize_t sz;

    while ((sz = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        write(pipe1[1], buf, sz);

        sz = read(pipe2[0], buf, sizeof(buf));
        write(STDOUT_FILENO, buf, sz);
    }

    // закрываем pipe'ы
    close(pipe1[1]);
    close(pipe2[0]);

    // ждём пока дети закончат свою работу
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return 0;
}