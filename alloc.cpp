#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ansi.h"

#ifdef _DEBUG
    #warning "_DEBUG is ENABLED"
    #define DEBUG_PRINT(...) \
        fprintf(stderr, __VA_ARGS__); fflush(stderr);
#else
    #define DEBUG_PRINT(...) (void)0
#endif

char * read_file_to_buf(char * filename, size_t * buf_len);

int main(int argc, char * argv[]) {
    if (argc < 2){
        fprintf(stderr, RED("U must provide file with onegin text in first cli argument!"));
        return 1;
    }

    size_t buf_len = 0;
    char * buf = read_file_to_buf(argv[1], &buf_len);

    if (buf == NULL) {
        printf("Произошла ошибка, смотри stderr");
        return 1;
    }

    printf("Было получено %zu байтов (с учетом \\0)", buf_len);

    printf("%s", buf);

    return 0;
}

char * read_file_to_buf(char * filename, size_t * buf_len) {
    int fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("Не удалось открыть файл");
        return NULL;
    }

    struct stat file_info = {};

    if (stat(filename, &file_info) == -1) {
        close(fd);
        perror("Не удалось получить информацию о файле");
        return NULL;
    }

    size_t byte_len = (size_t) file_info.st_size;

    // Добавляем +1 чтобы при вводе поместился '\0'
    char * buff = (char *) calloc(byte_len + 1, sizeof(char));

    if (buff == NULL) {
        close(fd);
        perror("Не удалось выделить память");
        return NULL;
    }

    if (read(fd, buff, byte_len) == -1) {
        close(fd);
        perror("Не удалось прочитать из файла");
        return NULL;
    }

    buff[byte_len] = '\0';
    * buf_len = byte_len + 1;

    close(fd);

    return buff;
}