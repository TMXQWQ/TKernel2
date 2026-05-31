#include "printk.h"
#include <string.h>

void panic(const char *format, ...){
    va_list args;
    va_start(args, format);
    vwprintf(&stdio, format, args);
    va_end(args);
}
