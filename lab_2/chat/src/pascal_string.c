//
// Created by xenon on 09.05.17.
//

#include <pascal_string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

pascal_string_t *pstrdup(pascal_string_t *str) {
    uint32_t len = sizeof(uint32_t) + str->length;
    pascal_string_t *dup = malloc(len);
    memcpy(dup, str, len);
    return dup;
}

char *p_str_to_c_str(pascal_string_t *p_str) {
    return strndup(p_str->data, p_str->length);
}

