#ifndef XENON_MD5_XENON_MD5_H
#define XENON_MD5_XENON_MD5_H

#include <stdint.h>

/* Return 128-bit (16-byte) array */
uint8_t *md5sum(const char *str);
char *md5hex(const char *str);

#endif /* XENON_MD5_XENON_MD5_H */
