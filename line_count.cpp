#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/stat.h>

#include "stringNthong.h"

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

int main(int argc, char * argv[]) {
    char * filename = NULL;
    if (argc == 2) {
        filename = argv[1];
    }
    else {
        printf("U must pass exactly 1 file name.");
    }
    FILE * file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file!\n");
        return -1;
    }

    size_t line_count = 0, max_line_len = 0, this_line_len = 0;
    int c = 0;
    while((c = getc(file)) != EOF) {
        if (c == '\n') {
            ++line_count;
            max_line_len = max_size_t(2, max_line_len, ++this_line_len);
            this_line_len = 0;
        }
        else
            ++this_line_len;
    }

    printf("line count is: %zu\nmax_line_len is: %zu\n", line_count, max_line_len);

    fclose(file);
    return 0;
}