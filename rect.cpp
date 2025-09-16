#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "ansi.h"

#ifdef _DEBUG
    #warning "_DEBUG is ENABLED"
    #define DEBUG_PRINT(...) \
        fprintf(stderr, __VA_ARGS__); fflush(stderr);
#else
    #define DEBUG_PRINT(...) (void)0
#endif

size_t max_size_t(size_t count, ...) {
    size_t max = 0;
    va_list ap;
    va_start(ap, count);
    for (size_t i = 0; i < count; ++i) {
        size_t ival = va_arg(ap, size_t);
        if (ival > max) max = ival;
    }
    return max;
}

size_t rect_arr_index(size_t y, size_t x, size_t max_y, size_t max_x);

char * get_one_onegin_string(FILE * file);
int is_empty_after_trim(const char *str);
int read_onegin(char * onegin_rect_arr, char * filename, size_t line_count, size_t max_line_len);
void write_onegin(char * onegin_rect_arr, size_t line_count, size_t max_line_len);
void save_onegin(char * filename, char * onegin_rect_arr, size_t line_count, size_t max_line_len);
void sort_onegin(char * onegin_rect_arr, size_t line_count, size_t max_line_len);
void strswp(char * str1, char * str2);

int main(int argc, char * argv[]) {
    if (argc < 2){
        fprintf(stderr, RED("U must provide file with onegin text in first cli argument!\n"));
        // exit(1);
        return 1;
        // abort();
    }
    char * filename = argv[1];
    const size_t max_line_len = 60, line_count = 810;

    char onegin_rect_arr[line_count][max_line_len+1] = {}; // чтобы поместился '\0'

    if (read_onegin((char *) onegin_rect_arr, filename, line_count, max_line_len))
        fprintf(stderr, RED("Can't open file!"));

    write_onegin((char *) onegin_rect_arr, line_count, max_line_len);
    printf("---------------------------------------------\n");
    sort_onegin((char *) onegin_rect_arr, line_count, max_line_len);

    write_onegin((char *) onegin_rect_arr, line_count, max_line_len);
    save_onegin("out_onegin.txt", (char *) onegin_rect_arr, line_count, max_line_len);
    return 0;
}

size_t rect_arr_index(size_t y, size_t x, size_t size_Y, size_t size_X) {
    return y * size_X + x;
}

int read_onegin(char * onegin_rect_arr, char * filename, size_t line_count, size_t max_line_len) {
    FILE * file = fopen(filename, "r");

    if (file == NULL) return 1;

    for (size_t y = 0; y < line_count; ++y) {
        DEBUG_PRINT("y is %zu, \tindex is %zu\n", y, y*(max_line_len+1));
        DEBUG_PRINT("pointer is %p\n", onegin_rect_arr + y*(max_line_len+1));
        DEBUG_PRINT("pointer is %p\n", onegin_rect_arr + y*(max_line_len+1));

        fgets(onegin_rect_arr + y*(max_line_len+1), (int) max_line_len+1, file);

        if (is_empty_after_trim(onegin_rect_arr + y*(max_line_len+1)))
            onegin_rect_arr[y*(max_line_len+1)] = '\0';
    }

    fclose(file);
    return 0;
}

void write_onegin(char * onegin_rect_arr, size_t line_count, size_t max_line_len) {
    for (size_t y = 0; y < line_count; ++y) {
        printf("(%s)", onegin_rect_arr + y*(max_line_len+1));
    }
}

void save_onegin(char * filename, char * onegin_rect_arr, size_t line_count, size_t max_line_len) {
    FILE * fp = fopen(filename, "w");
    for (size_t y = 0; y < line_count; ++y) {
        fprintf(fp, "(%s)", onegin_rect_arr + y*(max_line_len+1));
    }
    fclose(fp);
}

void sort_onegin(char * onegin_rect_arr, size_t line_count, size_t max_line_len) {
    for (size_t i = 0; i < line_count; ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (strcmp(onegin_rect_arr + i*(max_line_len+1),
                       onegin_rect_arr + j*(max_line_len+1)) < 0) {
                strswp(onegin_rect_arr + i*(max_line_len+1),
                       onegin_rect_arr + j*(max_line_len+1));
            }
        }
    }
}

void strswp(char * str1, char * str2) {
    size_t len = max_size_t(2, strlen(str1), strlen(str2));
    char * tmp_str = (char *) calloc(len+1, sizeof(char));
    strcpy(tmp_str, str1);
    strcpy(str1, str2);
    strcpy(str2, tmp_str);
    free(tmp_str);
}

// Проверяем есть ли в строке не пробельные символы
int is_empty_after_trim(const char *str) {
    if (!str) return 1;
    while (*str) {
        if (!isspace((unsigned char)*str))
            return 0;
        str++;
    }
    return 1;
}