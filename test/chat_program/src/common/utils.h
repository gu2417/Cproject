#ifndef UTILS_H
#define UTILS_H

#include <time.h>

/* Timestamp utilities */
char *format_timestamp(time_t timestamp);
char *time_ago(time_t timestamp);

/* String utilities */
void trim_whitespace(char *str);
void safe_strncpy(char *dest, const char *src, size_t dest_size);

/* Hash utilities */
unsigned int fnv1a_hash(const char *str);

#endif // UTILS_H
