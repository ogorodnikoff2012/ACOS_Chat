#include <tableprint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_table(const table_t *table) {
    row_t *row = table->first_row;
    while (row != NULL) {
        for (int i = 0; i < table->row_width; ++i) {
            if (i) {
                printf(" ");
            }
            const char *fmt = table->formatters[i];
            if (fmt == NULL) {
                fmt = "%*s";
            }
            printf(fmt, table->col_widths[i], row->entries[i]);
        }
        printf("\n");
        row = row->next_row;
    }
}

void free_table(table_t *table) {
    row_t *row = table->first_row;
    while (row != NULL) {
        for (int i = 0; i < table->row_width; ++i) {
            free(row->entries[i]);
        }

        free(row->entries);

        row_t *next_row = row->next_row;
        free(row);
        row = next_row;
    }
    free(table->col_widths);
    for (int i = 0; i < table->row_width; ++i) {
        if (table->formatters[i] != NULL) {
            free((void *) table->formatters[i]);
        }
    }

    free(table->formatters);
}

void init_table(table_t *table, int row_width) {
    table->first_row = NULL;
    table->last_row = NULL;
    table->col_widths = calloc(sizeof(int), row_width);
    table->formatters = calloc(sizeof(char *), row_width);
    table->row_width = row_width;
}

void table_set_formatter(table_t *table, int row, const char *fmt) {
    int len = strlen(fmt);
    char *str = calloc(sizeof(char), len + 1);
    strncpy(str, fmt, len);
    if (table->formatters[row] != NULL) {
        free((void *) table->formatters[row]);
    }
    table->formatters[row] = str;
}

static inline int max_i32(int a, int b) {
    return a < b ? b : a;
}

void add_row(table_t *table, ...) {
    va_list args;
    va_start(args, table);

    row_t *row = calloc(sizeof(row_t), 1);
    row->entries = calloc(sizeof(char *), table->row_width);

    for (int i = 0; i < table->row_width; ++i) {
        char *str = va_arg(args, char *);
        int len = strlen(str);
        row->entries[i] = calloc(sizeof(char), (len + 1));
        strncpy(row->entries[i], str, len);
        table->col_widths[i] = max_i32(table->col_widths[i], len);
    }

    if (table->last_row == NULL) {
        table->first_row = table->last_row = row;
    } else {
        table->last_row->next_row = row;
        table->last_row = row;
    }

    va_end(args);
}
