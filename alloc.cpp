#include <assert.h>
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
ssize_t count_needle_in_haystack(char * haystack, size_t haystack_len, char needle);
ssize_t replace_needle_in_haystack(char * haystack, size_t haystack_len, char src, char dst);
char ** split_buf_to_ptr_array(char * buf, size_t buf_len, size_t * line_count);

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

    printf("\n------------------------------\n");

    size_t line_count = 0;
    char ** ptr_array = split_buf_to_ptr_array(buf, buf_len, &line_count);

    for (size_t i = 0; i < line_count; ++i){
        printf("%s\n", ptr_array[i]);
    }

    printf("line count is %zu\n", line_count);

    free(buf);
    free(ptr_array);

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

ssize_t count_needle_in_haystack(char * haystack, size_t haystack_len, char needle) {
    assert(haystack != NULL     && "U must pass haystack to count needles");
    assert(needle != '\0'       && "U must pass needle other than '\\0'");

    if (haystack == 0) {
        perror("U must pass haystack to count needles");
        return -1;
    }

    if (needle == '\0') {
        perror("U must pass needle other than '\\0'");
        return -1;
    }

    ssize_t count = 0;
    char ch = 0;
    for (size_t i = 0; i < haystack_len; ++i) {
        ch = haystack[i];
        if (ch == '\0') return count;
        if (ch == needle) ++count;
    }
    return count; // Дошли до haystack_len но не встретили '\0'
}

ssize_t replace_needle_in_haystack(char * haystack, size_t haystack_len, char src, char dst) {
    assert(haystack != NULL     && "U must pass haystack to count needles");
    assert(src != '\0'          && "U must pass src other than '\\0'");

    if (haystack == 0) {
        perror("U must pass haystack to count needles");
        return -1;
    }

    if (src == '\0') {
        perror("U must pass src other than '\\0'");
        return -1;
    }

    ssize_t count = 0;
    char ch = 0;
    for (size_t i = 0; i < haystack_len; ++i) {
        ch = haystack[i];
        if (ch == '\0') return count;
        if (ch == src) {
            ++count;
            haystack[i] = dst;
        }
    }
    return count; // Дошли до haystack_len но не встретили '\0'
}

char ** split_buf_to_ptr_array(char * buf, size_t buf_len, size_t * line_count) {
    assert(buf != NULL      && "U must pass buffer to split them");

    ssize_t lc = -1; // Временная signed переменная чтобы не потерять -1
    if ((lc = replace_needle_in_haystack(buf, buf_len, '\n', '\0')) < 0) {
        perror("Negative line count received from" GREEN("replace_needle_in_haystack")
               ", see stderr above");
        return NULL;
    }
    *line_count = (size_t) lc;

    char ** ptr_array = (char **) calloc((*line_count) + 1, sizeof(void *));

    if (ptr_array == NULL) {
        perror("Не удалось выделить память");
        return NULL;
    }

    ptr_array[0] = &buf[0]; // Нулевой указатель указывает на нулевую строку

    size_t ptr_array_index = 1; // Потому что 0 уже указали
    int is_previous_zero = 0;
    for (size_t i = 0; i < buf_len; ++i) {
        // printf("%zu: Now ch is %c (%d), ptr_array_index is %zu, is_previous_zero is %d\n", i, buf[i], buf[i], ptr_array_index, is_previous_zero);
        // Если встретили \0 ставим флаг
        if (is_previous_zero == 0       && buf[i] == '\0') {
            is_previous_zero = 1;
        }
        // Когда встретили первый не \0 то значит нашли следующую строку
        else if (is_previous_zero == 1  && buf[i] != '\0') {
            is_previous_zero = 0;
            ptr_array[ptr_array_index] = &buf[i];
            ++ptr_array_index;
        }
    }

    return ptr_array;
}
