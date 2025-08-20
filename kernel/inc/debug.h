#pragma once

#include "vargs.h"
#include "stdint.h"

int panic(const char* msg,...);

int printk(const char* format,...);

int printks(const char* format,...);

int vsprintf(char *buff, const char *format, va_list args);

void plogk(const char* format,...);

char *number(char *str, int64_t num, int base, int size, int precision,
             int type);

int skip_atoi(const char **s);

int printkf(const char *format, ...) ; //强制格式化输出,不使用锁机制
