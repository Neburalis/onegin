#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "io_utils.h"
#include "stringNthong.h"

// Дополнение в терминале ?

using namespace mystr;

#define FREE(ptr)           \
    free(ptr); ptr = NULL;

struct String {
    char * start_ptr;
    char * end_ptr;
};

// func is allocate buffer, don't forgot free return value
char * read_file_to_buf(const char * const filename, size_t * const buf_len);

String * split_buf_to_ptr_array(char * const buf, const size_t buf_len, size_t * const line_count);

void output_strings_array_to_file
    (FILE * file, const size_t line_count, String * const strings_array);

void universal_swp(void * const ptr1, void * const ptr2, void * const temp, const size_t size);

void move_ptr_to_first_not_alpha_symbol(char ** ptr, int backword);

int string_compare_by_not_alpha_symbols(const String str1, const String str2, int backword);

size_t string_print(const String * const str, FILE * const file);

void sort_struct_onegin(String * const strings_array, const size_t line_count, int backword);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, RED("U must provide file with onegin text in first cli argument "
                            "and file to result in second cli argument!"));
        return 1;
    }
    else if (argc == 2) {
        fprintf(stderr, RED("U must provide file to result in second cli argument!"));
    }

    // ------------------------------------------------------------
    // Открытие файла для результата
    FILE * result_file = fopen(argv[2], "w");
    if (!result_file) {
        ERROR_MSG("Произошла ошибка при попытке открыть %s", argv[2]);
    }

    // ------------------------------------------------------------
    // Считывание онегина в буфер
    size_t buf_len = 0;
    char * buf = read_file_to_buf(argv[1], &buf_len);

    if (buf == NULL) {
        ERROR_MSG("Произошла ошибка, смотри stderr");
        return 1;
    }

    fprintf(result_file, "Было получено %zu байтов (с учетом \\0)", buf_len);

    // ------------------------------------------------------------
    // Разделение буфера на массив структур String

    size_t line_count = 0;
    String * strings_array = split_buf_to_ptr_array(buf, buf_len, &line_count);

    if (strings_array == NULL) {
        ERROR_MSG("Произошла ошибка, смотри stderr");
        return 1;
    }

    // ------------------------------------------------------------
    // Прямая сортировка (строим энциклопедию Русской жизни)

    fprintf(result_file, "\n------------------------------                     \n"
                         "Прямая сортировка (строим энциклопедию Русской жизни)\n\n");
    sort_struct_onegin(strings_array, line_count, 0); // forward sort
    output_strings_array_to_file(result_file, line_count, strings_array);

    // ------------------------------------------------------------
    // Обратная сортировка (строим обратный словарь | словарь рифм)

    fprintf(result_file, "\n------------------------------                            \n"
                         "Обратная сортировка (строим обратный словарь | словарь рифм)\n\n");
    sort_struct_onegin(strings_array, line_count, 1); // backward sort
    output_strings_array_to_file(result_file, line_count, strings_array);

    // ------------------------------------------------------------
    // Оригинальный текст (чтобы литераторы не съели)

    fprintf(result_file, "\n------------------------------              \n"
                         "Оригинальный текст (чтобы литераторы не съели)\n\n");
    fprintf(result_file, "%s\n", buf); // original onegin

    FREE(buf);
    FREE(strings_array);
    return 0;
}

// func is allocate buffer, don't forgot free return value
char * read_file_to_buf(const char * const filename, size_t * const buf_len) {
    assert(filename != NULL && "U must provide valid filename");
    assert(buf_len != NULL  && "U must provide valid ptr to buf_len");

    int fd = open(filename, O_RDONLY);

    if (fd == -1) {
        errno = ENOENT; // open не выставляет errno
        ERROR_MSG("Не удалось получить информацию о файле %s\n", filename);
        perror("");
        return NULL;
    }

    ssize_t byte_len = file_byte_size(filename);
    // signed чтобы не потерять отрицательное значение в случае ошибки
    if (byte_len <= 0) {
        // Сообщение об ошибке уже выдала file_byte_size
        return NULL;
    }

    // Добавляем +1 чтобы при вводе поместился '\0'
    char * buff = (char *) calloc((size_t) byte_len + 1, sizeof(char));
    // TODO don't alloc in func
    *buf_len = (size_t) byte_len + 1;

    if (buff == NULL) {
        close(fd);
        perror(""); // errno placed by calloc
        return NULL;
    }

    if (read(fd, buff, (size_t) byte_len) == -1) { // TODO: don't use too more sys calls
        close(fd);
        ERROR_MSG("Не прочитать из файла %s\n", filename);
        return NULL;
    }

    buff[byte_len] = '\0';

    close(fd);

    return buff;
}

String * split_buf_to_ptr_array(char * const buf, const size_t buf_len, size_t * const line_count) {
    assert(buf != NULL          && "U must pass buffer to split them");
    assert(line_count != NULL   && "U must provide valid ptr to buf_len");

    ssize_t lc = -1; // Временная signed переменная чтобы не потерять -1
    if ((lc = count_needle_in_haystack(buf, buf_len, '\n')) < 0) {
        ERROR_MSG("Negative line count have been received from" GREEN("replace_needle_in_haystack")
               ", see stderr above");
        return NULL;
    }
    *line_count = (size_t) lc + 1;
    // printf("line_count is %zu\n", *line_count);
    String * strings_array = (String *) calloc(*line_count + 1, sizeof(String));
    // printf("strings_array len is %zu, sizeof is %zu\n", (*line_count) + 1, sizeof(String));

    if (strings_array == NULL) {
        ERROR_MSG(""); // calloc уже указал errno
        return NULL;
    }

    strings_array[0] = {.start_ptr = &buf[0]};

    // Индекс в массиве строк
    // В 0 уже указан указатель на начало но еще нет указателя на конец
    ssize_t current_string_index = 0;
    // Флаг обработки
    int is_previous_finished = 0; // Когда в предыдущий запишем указатель на конец поменяем на 1
    size_t buf_index = 0;
    char * buf_ptr = buf;
    while (buf_index < buf_len) {
        if (is_previous_finished == 0) {
            buf_ptr = strchr(buf_ptr, '\n'); // Находим конец текущей строки
            if (buf_ptr == NULL) {
                strings_array[current_string_index].end_ptr = buf + buf_len;
                // Дошли до конца буфера и не встретили \n - значит обработали последнюю строку
                break;
            }
            ++buf_ptr; // сейчас указываем на \n
            strings_array[current_string_index].end_ptr = buf_ptr;
            buf_index = (size_t)(buf_ptr - buf); // Вычисляем текущий индекс в массиве
            is_previous_finished = 1;
            ++current_string_index;
        }
        if (is_previous_finished == 1) {
            while (isspace(*buf_ptr)) ++buf_ptr; // Находим ближайший не пробельный символ
            if (*buf_ptr == '\0') {
                // дошли до конца строки не встретив не пробелы,
                // значит в конце файла были пустые строки их не сохраняем
                // Последнее увеличение было лишним и все строки уже обработаны
                --current_string_index;
                break;
            }
            // Указываем на найденный символ
            strings_array[current_string_index].start_ptr = buf_ptr;
            is_previous_finished = 0; // Еще не нашли конец текущей строки
        }
    }

    *line_count = (size_t) current_string_index;
    strings_array = (String *) realloc(strings_array,
                                ((size_t) current_string_index + 1) * sizeof(strings_array[0]));

    if (strings_array == NULL) {
        ERROR_MSG(""); // realloc уже указал errno
        return NULL;
    }

    return strings_array;
}

void output_strings_array_to_file
    (FILE * file, const size_t line_count, String * const strings_array) {
    assert(strings_array            != NULL  && "strings_array must be not NULL ptr");
    assert(strings_array->start_ptr != NULL  && "strings_array must be contain valid string start");
    assert(strings_array->end_ptr   != NULL  && "strings_array must be contain valid string end");

    for (size_t i = 0; i < line_count; ++i) {
        fprintf(file, "%zu\t| ", i);
        string_print(&strings_array[i], file);
    }
    fprintf(file, "line count is %zu\n", line_count);
}

void universal_swp(void * const ptr1, void * const ptr2, void * const temp, const size_t size) {
    assert(ptr1 != NULL     && "Ptr1 must be not null pointer");
    assert(ptr2 != NULL     && "Ptr2 must be not null pointer");
    assert(temp != NULL     && "temp must be not null pointer");

    memcpy(temp, ptr1, size);
    memcpy(ptr1, ptr2, size);
    memcpy(ptr2, temp, size);
}

void move_ptr_to_first_not_alpha_symbol(char ** ptr, int backword) {
    assert(ptr != NULL);
    assert(*ptr != NULL);

    while (**ptr != '\0' && !isalpha(**ptr)) {
        if (backword)
            --(*ptr);
        else // forward
            ++(*ptr);
    }
}

int string_compare_by_not_alpha_symbols(const String str1, const String str2, int forward) {
    assert(str1.start_ptr   != NULL    && "str1 start must be point to not null pointer (string)");
    assert(str1.end_ptr     != NULL    && "str1 end must be point to not null pointer");
    assert(str2.start_ptr   != NULL    && "str2 start must be point to not null pointer (string)");
    assert(str2.end_ptr     != NULL    && "str2 end must be point to not null pointer");

    char * start_ptr1 = str1.start_ptr;
    char * start_ptr2 = str2.start_ptr;
    char * end_ptr1 = str1.end_ptr;
    char * end_ptr2 = str2.end_ptr;

    if (forward)
        for (;;) {
            move_ptr_to_first_not_alpha_symbol(&start_ptr1, 0);
            move_ptr_to_first_not_alpha_symbol(&start_ptr2, 0);
            // Теперь *str1 и *str2 - точно буквы

            // Обе строки закончились -> одинаковые
            if (*start_ptr1 == '\0' && *start_ptr2 == '\0') return 0;
            // Закончилась первая -> она короче
            if (*start_ptr1 == '\0') return -1;
            // Закончилась вторая -> она короче
            if (*start_ptr2 == '\0') return 1;

            if (tolower(*start_ptr1) != tolower(*start_ptr2))
                return tolower(*start_ptr2) - tolower(*start_ptr1);

            ++start_ptr1;
            ++start_ptr2;
        }
    else // backword
        for (;;) {
            move_ptr_to_first_not_alpha_symbol(&end_ptr1, 1);
            move_ptr_to_first_not_alpha_symbol(&end_ptr2, 1);
            // Теперь *str1 и *str2 - точно буквы

            if (*end_ptr1 == '\0' && *end_ptr2 == '\0') return 0;   // Обе строки закончились -> одинаковые
            if (*end_ptr1 == '\0') return -1;                   // Закончилась первая -> она короче
            if (*end_ptr2 == '\0') return 1;                    // Закончилась вторая -> она короче

            if (tolower(*end_ptr1) != tolower(*end_ptr2))
                return tolower(*end_ptr2) - tolower(*end_ptr1);

            --end_ptr1;
            --end_ptr2;
        }
}

size_t string_print(const String * const str, FILE * const file) {
    assert(str            != NULL     && "strings_array must be not NULL ptr");
    assert(str->start_ptr != NULL     && "strings_array must be contain valid string start");
    assert(str->end_ptr   != NULL     && "strings_array must be contain valid string end");
    assert(file           != NULL     && "file must be not NULL");

    size_t count = 0;
#ifdef _DEBUG // Для дебага выводим много доп инфы
    fprintf(file, "(%p):(%p)[", str, str->start_ptr);
    for (char * ptr = str->start_ptr; ptr < str->end_ptr - 1; ++ptr) {
        putc(*ptr, file);
        ++count;
    }
    fprintf(file, "](%p)(len=%ld)\n", str->end_ptr, str->end_ptr-str->start_ptr);
#else // Без дебага пишем чисто строку
    for (char * ptr = str->start_ptr; ptr < str->end_ptr - 1; ++ptr) {
        putc(*ptr, file);
        ++count;
    }
    putc('\n', file);
#endif
    return count;
}

void sort_struct_onegin(String * const strings_array, const size_t line_count, int backword) {
    assert(strings_array            != NULL     && "strings_array must be not NULL ptr");
    assert(strings_array->start_ptr != NULL     && "strings_array must be contain valid string start");
    assert(strings_array->end_ptr   != NULL     && "strings_array must be contain valid string end");

    auto forward_string_compare = [](const void * vstr1, const void * vstr2) {
        const String str1 = *((const String *) vstr1);
        const String str2 = *((const String *) vstr2);
        return -string_compare_by_not_alpha_symbols(str1, str2, 1);
    };
    auto backword_string_compare = [](const void * vstr1, const void * vstr2) {
        const String str1 = *((const String *) vstr1);
        const String str2 = *((const String *) vstr2);
        return -string_compare_by_not_alpha_symbols(str1, str2, 0);
    };

    if (backword) {
        qsort(strings_array, line_count, sizeof(strings_array[0]), (backword_string_compare));
    }
    else { // forward
        qsort(strings_array, line_count, sizeof(strings_array[0]), (forward_string_compare));
    }
}
