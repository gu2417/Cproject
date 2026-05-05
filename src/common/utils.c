#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

char *format_timestamp(time_t timestamp)
{
    static char buf[32];
    struct tm *tm_info = localtime(&timestamp);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

char *time_ago(time_t timestamp)
{
    static char buf[64];
    time_t now = time(NULL);
    long diff = now - timestamp;

    if (diff < 60) {
        snprintf(buf, sizeof(buf), "방금 전");
    } else if (diff < 3600) {
        snprintf(buf, sizeof(buf), "%ld분 전", diff / 60);
    } else if (diff < 86400) {
        snprintf(buf, sizeof(buf), "%ld시간 전", diff / 3600);
    } else {
        snprintf(buf, sizeof(buf), "%ld일 전", diff / 86400);
    }

    return buf;
}

void trim_whitespace(char *str)
{
    if (!str) {
        return;
    }

    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    char *end = str + strlen(str) - 1;
    while (end >= start && isspace((unsigned char)*end)) {
        end--;
    }

    size_t len = (end - start) + 1;
    if (start > str) {
        memmove(str, start, len);
    }

    str[len] = '\0';
}

void safe_strncpy(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) {
        return;
    }

    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

unsigned int fnv1a_hash(const char *str)
{
    unsigned int hash = 2166136261u;

    if (!str) {
        return hash;
    }

    while (*str) {
        hash ^= (unsigned char)*str;
        hash *= 16777619;
        str++;
    }

    return hash;
}
