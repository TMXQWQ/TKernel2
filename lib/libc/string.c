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

size_t strlen(const char *str)
{
#if defined(__builtin_strlen)
    return __builtin_strlen(str);
#else
    size_t len = 0;
    while (*str++ != '\0') len++;
    return len;
#endif
}

/* Copy n bytes from memory area str2 to memory area str1 */
void *memcpy(void *str1, const void *str2, size_t n)
{
#if defined(__builtin_memcpy)
    __builtin_memcpy(str1, str2, n);
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("rep movsb" ::"D"(str1), "S"(str2), "c"(n) : "memory");
#else
    volatile uint8_t       *dest = (volatile uint8_t *)str1;
    const volatile uint8_t *src  = (const volatile uint8_t *)str2;
    const volatile uint8_t *end  = (const volatile uint8_t *)((uint8_t *)str2 + n);

    if (dest == src) return str1;
    while (src != end) *dest++ = *src++;
#endif
    return str1;
}

/* Sets a memory area to the specified value */
void *memset(void *str, int c, size_t n) // NOLINT
{
#if defined(__builtin_memset)
    __builtin_memset(str, c, n);
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("rep stosb" ::"D"(str), "a"(c), "c"(n) : "memory");
#else
    volatile uint8_t *_str = (volatile uint8_t *)str;
    volatile uint8_t *end  = (volatile uint8_t *)((uint8_t *)str + n);
    const uint8_t     _c   = c;

    for (; _str < end; _str++) *_str = _c;
#endif
    return str;
}
