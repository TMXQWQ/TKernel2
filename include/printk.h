/*
 *
 *      printk.h
 *      Kernel string print header file
 *
 *      2024/6/27 By Rainy101112
 *      Based on Apache 2.0 open source license.
 *      Copyright © 2020 ViudiraTech, based on the Apache 2.0 license.
 *
 */

#ifndef INCLUDE_PRINTK_H_
#define INCLUDE_PRINTK_H_

#include "stdarg.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"

#ifndef KERNEL_LOG
#    define KERNEL_LOG 1
#endif

#define ansi_black   0
#define ansi_red     1
#define ansi_green   2
#define ansi_yellow  3
#define ansi_blue    4
#define ansi_magenta 5
#define ansi_cyan    6
#define ansi_white   7

#define ansi_normal     0
#define ansi_bold       1
#define ansi_underline  4
#define ansi_shine      5
#define ansi_shine_fast 6
#define ansi_delline    9

#define __ansi_tmp_macro1(x) #x
#define __ansi_tmp_macro2(x) __ansi_tmp_macro1(x)
#define ansi_color(info, background, frontground, flags) \
    "\e["__ansi_tmp_macro2(flags) ";3"__ansi_tmp_macro2(frontground) ";4"__ansi_tmp_macro2(background) "m" info "\e[0m"

// clang-format off
// 默认内核输出日志开头的ansi序列
#define ansi_V(info) ansi_color(info, ansi_black, ansi_magenta, ansi_bold)
// clang-format on

extern char   *plogk_info_stack[32];
extern uint8_t plogk_info_ptr;

typedef enum {
    OFLOW_AT_FMTARG,
    OFLOW_AT_FMTSTR,
} overflow_kind_t;

typedef struct {
        uint64_t size;       // The size of the buff to write
        char    *buff;       // The buff to write
        char    *last_write; // The last write position
} fmt_arg_t;

typedef struct {
        overflow_kind_t kind; // The kind of overflow
        fmt_arg_t      *arg;  // The argument that overflow
} overflow_signal_t;

typedef struct {
        char  *buf;
        size_t idx;
} unsafe_buf_data;

typedef struct {
        const char **fmt_ptr;       // a pointer to `fmt`
        size_t      *write_counter; // for `%n`
} args_fmter;

extern writer stdio;

/* Kernel print string */
void printk(const char *format, ...);

/* Kernel print log */
void plogk(const char *format, ...);

/* Handler of unsafe buf writing */
uint8_t unsafe_buf_write(writer *writer, char c);

/* Store the formatted output in a character array */
int sprintf(char *str, const char *fmt, ...);

/* Format with va_list, then store the formatted output in a character array */
int vsprintf(char *str, const char *fmt, va_list args);

/* Formatted output processing */
void wfmt_arg(writer *writer, args_fmter *fmter, va_list args);

/* Use a `writer` to write formatted string */
size_t vwprintf(writer *writer, const char *fmt, va_list args);

#endif // INCLUDE_PRINTK_H_
