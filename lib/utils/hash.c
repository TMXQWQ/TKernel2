#include "hash.h"

unsigned long easy_str_hash(const char *str)
{
    unsigned long hash = 5381;
    int           c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}