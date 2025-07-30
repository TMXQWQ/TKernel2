#pragma once

#include "vargs.h"

int panic(const char* msg,...);

int printk(const char* format,...);

int printks(const char* format,...);

int vsprintf(char *buff, const char *format, va_list args);
