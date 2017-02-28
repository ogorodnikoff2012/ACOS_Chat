#ifndef XENON_TABLEPRINT_H
#define XENON_TABLEPRINT_H

typedef struct row {
    char **entries;
    struct row *next_row;
} row_t;

typedef struct table {
    row_t *first_row, *last_row;
    int *col_widths;
    const char **formatters;
    int row_width;
} table_t;

void print_table(const table_t *table);
void free_table(table_t *table);
void init_table(table_t *table, int row_width);
void table_set_formatter(table_t *table, int row, const char *fmt);
void add_row(table_t *table, ...);

#endif // XENON_TABLEPRINT_H

