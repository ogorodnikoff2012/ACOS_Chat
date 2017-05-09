#ifndef XENON_CHAT_PASCAL_STRING_H
#define XENON_CHAT_PASCAL_STRING_H

typedef struct {
    int length;
    char data[];
} pascal_string_t;

pascal_string_t *pstrdup(pascal_string_t *str);
char *p_str_to_c_str(pascal_string_t *p_str);

#endif /* XENON_CHAT_PASCAL_STRING_H */
