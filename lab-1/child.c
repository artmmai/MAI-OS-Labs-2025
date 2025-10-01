#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>

int main(int argc, char* argv[]){
    if (argc < 2){
        const char msg[] = "error: not enough arguments, specify child id(1 or 2)\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    // argv[1][0] ([0] - "./child" ; [1] - 1 or 2)
    int child_number = argv[1][0] - '0'; // получаем 1-ый или 2-ой дочерний процесс
    char buf[4096];
    ssize_t sz;

    while ((sz = read(STDIN_FILENO, buf, sizeof(buf))) > 0){
        if (child_number == 1){
            // перевод в верхний регистр
            for (ssize_t i = 0; i < sz; ++i){
                buf[i] = toupper(buf[i]);
            }
            write(STDOUT_FILENO, buf, sz);
        }
        else {
            // замена пробельных символов на '_'
            for (ssize_t i = 0; i < sz; ++i){
                if (buf[i] == ' ' || buf[i] == '\t')
                    buf[i] = '_';
            }
            
            write(STDOUT_FILENO, buf, sz);
        }
    }
    return 0;

}