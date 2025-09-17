#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define ERROR_MSG(format, ...) \
    fprintf(stderr, \
            "In " GREEN("%s:%d") ", " YELLOW("%s") ".\n" format "\n", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__); fflush(stderr);

ssize_t file_byte_size(const char * const filename);
char * read_file_to_buf(char * filename, size_t * buf_len);
ssize_t count_needle_in_haystack(char * haystack, size_t haystack_len, char needle);
ssize_t replace_needle_in_haystack(char * haystack, size_t haystack_len, char src, char dst);
char ** split_buf_to_ptr_array(char * buf, size_t buf_len, size_t * line_count);
void sort_ptr_onegin(char ** ptr_array, size_t line_count);

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
    printf("\n------------------------------\n");

    size_t line_count = 0;
    char ** ptr_array = split_buf_to_ptr_array(buf, buf_len, &line_count);

    if (ptr_array == NULL) {
        printf("Произошла ошибка, смотри stderr");
        return 1;
    }

    for (size_t i = 0; i < line_count; ++i){
        printf("%zu (%p):[%s]\n", i, ptr_array[i], ptr_array[i]);
    }

    printf("line count is %zu\n", line_count);

    printf("\n------------------------------\n");
    sort_ptr_onegin(ptr_array, line_count);

    for (size_t i = 0; i < line_count; ++i){
        printf("%zu (%p):[%s]\n", i, ptr_array[i], ptr_array[i]);
    }

    free(buf);
    free(ptr_array);
    return 0;
}

ssize_t file_byte_size(const char * const filename) {
    struct stat file_info = {};

    if (stat(filename, &file_info) == -1) {
        fprintf(stderr, "Не удалось получить информацию о файле %s\n", filename);
        perror("");
        return -1;
    }

    return (ssize_t) file_info.st_size;
}

char * read_file_to_buf(char * filename, size_t * buf_len) {
    int fd = open(filename, O_RDONLY);

    if (fd == -1) {
        fprintf(stderr, "Не удалось получить информацию о файле %s\n", filename);
        perror("");
        return NULL;
    }

    ssize_t byte_len = 0;
    if ((byte_len = file_byte_size(filename)) <= 0) {
        // Сообщение об ошибке уже выдала file_byte_size
        return NULL;
    }


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

    *line_count = ptr_array_index;
    ptr_array = (char **) realloc(ptr_array, (ptr_array_index + 1) * sizeof(ptr_array));

    return ptr_array;
}

void ptr_swp(char ** ptr1, char ** ptr2) {
    assert(ptr1 != NULL     && "Ptr1 must be not null pointer");
    assert(ptr2 != NULL     && "Ptr2 must be not null pointer");
    assert(*ptr1 != NULL    && "Ptr1 must be point to not null pointer (string)");
    assert(*ptr2 != NULL    && "Ptr2 must be point to not null pointer (string)");

    char * tmp_ptr = *ptr1;
    *ptr1 = *ptr2;
    *ptr2 = tmp_ptr;
}

inline char * move_to_not_alpha(char * str) {
    assert(str != NULL  && "str must be not null ptr (string)");

    while(!isalpha(*str)) ++str;
    return str;
}

inline int string_compare_by_not_alpha_symbols(const char * str1, const char * str2) {
    assert(*str1 != NULL    && "str1 must be point to not null pointer (string)");
    assert(*str2 != NULL    && "str2 must be point to not null pointer (string)");

    for (;;) {
        while (*str1 != '\0' && !isalpha(*str1)) ++str1;
        while (*str2 != '\0' && !isalpha(*str2)) ++str2;
        // Теперь *str1 и *str2 - точно буквы

        if (*str1 == '\0' && *str2 == '\0') return 0;   // Обе строки закончились -> одинаковые
        if (*str1 == '\0') return -1;                   // Закончилась первая -> она короче
        if (*str2 == '\0') return 1;                    // Закончилась вторая -> она короче

        if (tolower(*str1) != tolower(*str2)) return tolower(*str2) - tolower(*str1);

        ++str1;
        ++str2;
    }
}

void sort_ptr_onegin(char ** ptr_array, size_t line_count) {
    assert(ptr_array != NULL     && "ptr_array must be not null pointer");
    assert(*ptr_array != NULL    && "ptr_array must be point to not null pointer (string)");

    for (size_t i = 0; i < line_count; ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (string_compare_by_not_alpha_symbols(move_to_not_alpha(ptr_array[i]),
                                                    move_to_not_alpha(ptr_array[j])) > 0)
                ptr_swp(&ptr_array[i], &ptr_array[j]);
        }
    }
}
