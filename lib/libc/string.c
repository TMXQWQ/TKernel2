#include "string.h"
#include "stdint.h"

int strcmp(const char *str1, const char *str2)
{
#if defined(__builtin_strcmp)
    return __builtin_strcmp(str1, str2);
#else
    const uint8_t *_str1 = (const uint8_t *)str1;
    const uint8_t *_str2 = (const uint8_t *)str2;
    int            c1, c2;

    do {
        c1 = *_str1++;
        c2 = *_str2++;
        if (!c1) return c1 - c2;
    } while (c1 == c2);
    return c1 - c2;
#endif
}

/* Searches the string pointed to by the parameter str for the last occurrence of the character c */
char *strrchr(const char *str, int c)
{
#if defined(__builtin_strrchr)
    return __builtin_strrchr(str, c);
#else
    const char *finded = 0;
    for (; *str != '\0'; str++) {
        if (*str == c) finded = str;
    }
    return (char *)finded;
#endif
}
