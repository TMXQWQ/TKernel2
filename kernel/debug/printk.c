/*
 *
 *      printk.c
 *      Kernel string printing
 *
 *      2024/6/27 By Rainy101112
 *      Based on Apache 2.0 open source license.
 *      Copyright © 2020 ViudiraTech, based on the Apache 2.0 license.
 *
 */

#include "printk.h"
#include "serial.h"
#include "stdarg.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"

#define BUF_SIZE 2048 // least 2 bytes (1 byte is for '\0')

writer  stdio;
char   *plogk_info_stack[32] = {"INFO"};
uint8_t plogk_info_ptr       = 0;

/* Kernel print string */
void printk(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (!stdio.handler) {
        // write_serial(SERIAL_PORT_1, '!');
        while(1);
    }
    vwprintf(&stdio, format, args);
    va_end(args);
}

/* plogk / klog_printf / klog_dump 的实现见文件末尾（须置于 vwprintf 定义之后）。 */


/* Handler of unsafe buf writing */
uint8_t unsafe_buf_write(writer *writer, char c)
{
    unsafe_buf_data *data = (unsafe_buf_data *)writer->data;
    data->buf[data->idx]  = c;
    ++data->idx;
    return 1; // Always success? :(
}

/* Store the formatted output in a character array */
int sprintf(char *str, const char *fmt, ...)
{
    int             c                 = 0;
    unsafe_buf_data unsafe_buf_data   = {.buf = str, .idx = 0};
    writer          unsafe_buf_writer = {
        .data    = &unsafe_buf_data,
        .handler = unsafe_buf_write,
    };
    va_list arg;
    va_start(arg, fmt);

    c = (int)vwprintf(&unsafe_buf_writer, fmt, arg);
    unsafe_buf_writer.handler(&unsafe_buf_writer, '\0');

    va_end(arg);
    return c;
}

/* Format with va_list, then store the formatted output in a character array */
int vsprintf(char *str, const char *fmt, va_list args)
{
    int             c                 = 0;
    unsafe_buf_data unsafe_buf_data   = {.buf = str, .idx = 0};
    writer          unsafe_buf_writer = {
        .data    = &unsafe_buf_data,
        .handler = unsafe_buf_write,
    };
    c = (int)vwprintf(&unsafe_buf_writer, fmt, args);
    unsafe_buf_writer.handler(&unsafe_buf_writer, '\0');
    return c;
}

typedef enum num_size {
    HALF_2 = 0, // char
    HALF_1 = 1, // short
    INT    = 2, // int
    LONG_1 = 3, // long
    LONG_2 = 4, // long long
    SIZE_T = 5, // size_t
} num_size_t;

/* Formatted output processing */
void wfmt_arg(writer *writer, args_fmter *fmter, va_list args)
{
    char         *str           = 0; // for `%s`
    size_t        write_counter = 0;
    size_t        str_len       = 0; // for align `%s`
    const char  **fmt_ptr       = fmter->fmt_ptr;
    write_handler write         = writer->handler;

    num_formatter_t num_fmter = {};
    num_fmt_type    num_flag  = {};
    int8_t          size_cnt  = INT;

    /* Error args */
    if (!writer || !write || !fmt_ptr || !(*fmt_ptr) || **fmt_ptr != '%') return;

    while (1) {
        ++(*fmt_ptr); // Skip '%' or any flags
        switch (**fmt_ptr) {
            case '-' :
                num_flag.left = 1;
                break;
            case '+' :
                num_flag.plus = 1;
                break;
            case ' ' :
                num_flag.space = 1;
                break;
            case '#' :
                num_flag.special = 1;
                break;
            case '0' :
                num_flag.zeropad = 1;
                break;
            default :
                break;
        }

        /* Calc num_fmter.size */
        if (IS_DIGIT(**fmt_ptr)) {
            num_fmter.size = skip_atoi(fmt_ptr);
        } else if (**fmt_ptr == '*') {
            /* by the following argument */
            ++(*fmt_ptr); // Skip '*'
            num_fmter.size = (size_t)va_arg(args, int);
        }

        /* Calc num_fmter.precision */
        if (**fmt_ptr == '.') {
            ++(*fmt_ptr); // Skip '.'
            if (IS_DIGIT(**fmt_ptr)) {
                num_fmter.precision = skip_atoi(fmt_ptr);
            } else if (**fmt_ptr == '*') {
                /* by the following argument */
                ++(*fmt_ptr); // Skip '*'
                num_fmter.precision = (size_t)va_arg(args, int);
            }
        }

        /* Calc size_cnt */
        switch (**fmt_ptr) {
            case 'h' :
                size_cnt--;
                if (size_cnt < HALF_2) size_cnt = HALF_2; // hh
                continue;
            case 'L' :      // += 2
                size_cnt++; // fallthrough
            case 'l' :
                size_cnt++;
                if (size_cnt > LONG_2) size_cnt = LONG_2; // ll
                continue;
            case 'z' :
                size_cnt = SIZE_T; // z
                continue;
            default :
                break;
        }

        /* Read argument */
        switch (**fmt_ptr) {
            case 'c' :
                num_fmter.num = va_arg(args, int);
                break;
            case 's' :
                str                    = va_arg(args, char *);
                static char null_str[] = "(null)";
                if (str == 0) str = null_str;
                break;
            case 'd' :
            case 'i' :
                switch (size_cnt) {
                    case HALF_2 :
                        num_fmter.num = (size_t)(char)va_arg(args, int);
                        break;
                    case HALF_1 :
                        num_fmter.num = (size_t)(short)va_arg(args, int);
                        break;
                    case INT :
                        num_fmter.num = (size_t)(int)va_arg(args, int);
                        break;
                    case LONG_1 :
                        num_fmter.num = (size_t)(long)va_arg(args, long);
                        break;
                    case LONG_2 :
                        num_fmter.num = (size_t)(long long)va_arg(args, long long);
                        break;
                    case SIZE_T : // fallthrough
                    default :
                        num_fmter.num = va_arg(args, size_t);
                        break;
                }
                break;
            case 'o' :
            case 'x' :
            case 'X' :
            case 'b' :
            case 'u' :
                switch (size_cnt) {
                    case HALF_2 :
                        num_fmter.num = (size_t)(unsigned char)va_arg(args, int);
                        break;
                    case HALF_1 :
                        num_fmter.num = (size_t)(unsigned short)va_arg(args, int);
                        break;
                    case INT :
                        num_fmter.num = (size_t)(unsigned int)va_arg(args, int);
                        break;
                    case LONG_1 :
                        num_fmter.num = (size_t)(unsigned long)va_arg(args, long);
                        break;
                    case LONG_2 :
                        num_fmter.num = (size_t)(unsigned long long)va_arg(args, long long);
                        break;
                    case SIZE_T : // fallthrough
                    default :
                        num_fmter.num = va_arg(args, size_t);
                        break;
                }
                break;
            case 'p' :
                num_fmter.num = (size_t)va_arg(args, void *);
                break;
            default : // may no data
                break;
        }

        /* Calc length of `%s` and set num_flag */
        switch (**fmt_ptr) {
            case 'c' :
                break;
            case 's' :
                str_len = strlen(str);
                if (num_fmter.size < str_len) num_fmter.size = str_len;
                break;
            case 'o' :
                num_fmter.base = 8;
                break;
            case 'p' :
                num_flag.small   = 1;
                num_flag.special = 1;
                num_flag.zeropad = 1;
                if (num_fmter.size < 16) num_fmter.size = 16;
                num_fmter.base = 16;
                break;
            case 'x' :
                num_flag.small = 1; // fallthrough
            case 'X' :
                num_fmter.base = 16;
                break;
            case 'd' :
            case 'i' :
                num_flag.sign = 1; // fallthrough
            case 'u' :
                num_fmter.base = 10;
                break;
            case 'b' :
                num_fmter.base = 2;
                break;
            case 'n' :
                *(fmter->write_counter) += write_counter;
                *(int *)va_arg(args, void *) = (int)*fmter->write_counter;
                break;
            case '%' :
                break;
            default :
                /* Unexpected */
                return;
        }

        /* Write to arg space */
        switch (**fmt_ptr) {
            case 'c' :
                /* Right align */
                if (!(num_flag.left)) {
                    while (write_counter < num_fmter.size - 1) {
                        write(writer, ' ');
                        ++write_counter;
                    }
                }

                /* Write char */
                write(writer, (char)num_fmter.num);

                /* Left align */
                if (num_flag.left) {
                    while (write_counter < num_fmter.size - 1) {
                        write(writer, ' ');
                        ++write_counter;
                    }
                }
                break;
            case 's' :

                /* Right align */
                if (!(num_flag.left)) {
                    while (write_counter < num_fmter.size - str_len) {
                        write(writer, ' ');
                        ++write_counter;
                    }
                    str_len = num_fmter.size;
                }

                /* Write string */
                while (write_counter < str_len) {
                    write(writer, *str);
                    ++str;
                    ++write_counter;
                }

                /* Left align */
                if (num_flag.left) {
                    while (write_counter < num_fmter.size - str_len) {
                        write(writer, ' ');
                        ++write_counter;
                    }
                }
                break;
            case 'o' : // fallthrough
            case 'p' : // fallthrough
            case 'x' : // fallthrough
            case 'X' : // fallthrough
            case 'd' : // fallthrough
            case 'i' : // fallthrough
            case 'u' : // fallthrough
            case 'b' :
                write_counter += wnumber(writer, num_fmter, num_flag);
                break; // Format number with `writer`
            case '%' :
                write(writer, '%');
                break;
            default :
                break;
        }
        break;
    }
    *(fmter->write_counter) += write_counter;
    /* Unnecessary to update `fmt_ptr` */
}

/* Use a `writer` to write formatted string */
size_t vwprintf(writer *writer, const char *fmt, va_list args)
{
    const char   *fmt_ptr = fmt;
    size_t        result  = 0;
    write_handler write   = writer->handler;

    args_fmter fmter = {
        .fmt_ptr       = &fmt_ptr,
        .write_counter = &result,
    };

    while (*fmt_ptr != '\0') {
        if (*fmt_ptr != '%') {
            write(writer, *fmt_ptr); // TODO: Catch Error
            fmt_ptr++;
            result++;
            continue;
        }

        /* *fmt_ptr == '%' */
        wfmt_arg(writer, &fmter, args);
        fmt_ptr++;
    }
    return result;
}

/* ================= 内核日志环形缓冲 =================
 * 设计：普通 plogk 同时写环形缓冲 + 刷串口（保留启动日志可见）；
 *      klog_printf 只写环形缓冲、不刷串口（syscall 等高频噪音走它，
 *      避免污染 sh 所在的 COM1 控制台）。需要回看时调用 klog_dump()
 *     把缓冲全部刷到串口（panic/调试用）。 */
#define KLOG_RING_SIZE 65536
static char   klog_ring[KLOG_RING_SIZE];
static size_t klog_prod = 0;   /* 写指针（不取模，写入时取模） */
static size_t klog_cons = 0;   /* 读指针（dump 用） */

static void klog_ring_write(const char *s, size_t n) {
    for (size_t i = 0; i < n && s[i]; i++) {
        klog_ring[klog_prod & (KLOG_RING_SIZE - 1)] = s[i];
        klog_prod++;
    }
}

static void serial_puts(const char *s, size_t n) {
    if (!stdio.handler) return;
    for (size_t i = 0; i < n; i++) stdio.handler(&stdio, s[i]);
}
static void serial_puts_str(const char *s) { serial_puts(s, strlen(s)); }

/* 内存写回的 writer（把格式化结果写进本地缓冲区，便于先缓冲再决定是否刷串口） */
typedef struct { char *buf; int idx; } klog_mem_t;
static uint8_t klog_mem_write(writer *w, char c) {
    klog_mem_t *d = (klog_mem_t *)w->data;
    d->buf[d->idx++] = c;
    return 1;
}

/* 把带 [prefix] 头的行格式化进 buf（纯文本，不含 ANSI 转义） */
static void klog_format(char *buf, size_t cap, const char *format, va_list ap) {
    char hdr[64];
    int hl = sprintf(hdr, "[ %s ] ", plogk_info_stack[plogk_info_ptr]);
    if (hl < 0) hl = 0;
    if ((size_t)hl > cap - 2) hl = cap - 2;
    memcpy(buf, hdr, (size_t)hl);
    klog_mem_t d = { .buf = buf + hl, .idx = 0 };
    writer w = { .data = &d, .handler = klog_mem_write };
    vwprintf(&w, format, ap);
    buf[hl + d.idx] = '\0';
}

/* 普通内核日志：进环形缓冲 + 刷串口（[prefix] 头着色，正文默认色） */
void plogk(const char *format, ...) {
#if KERNEL_LOG
    va_list args; va_start(args, format);
    char buf[BUF_SIZE];
    klog_format(buf, BUF_SIZE, format, args);
    size_t len = strlen(buf);
    klog_ring_write(buf, len);
    /* 仅给 [prefix] 头着色，正文保持默认色（与原仓库行为一致，
     * 避免整行被 \e[1;35;40m 裹成前景品红+背景黑）。 */
    char hdr[64];
    int hl = sprintf(hdr, "[ %s ] ", plogk_info_stack[plogk_info_ptr]);
    if (hl < 0) hl = 0;
    serial_puts_str("\e[1;35;40m");
    serial_puts(hdr, (size_t)hl);
    serial_puts_str("\e[0m");
    serial_puts(buf + hl, len - (size_t)hl);
    va_end(args);
#else
    (void)format;
#endif
}

/* 静默日志：仅进环形缓冲，不刷串口（syscall 高频噪音用） */
void klog_printf(const char *format, ...) {
#if KERNEL_LOG
    va_list args; va_start(args, format);
    char buf[BUF_SIZE];
    klog_format(buf, BUF_SIZE, format, args);
    klog_ring_write(buf, strlen(buf));
    va_end(args);
#else
    (void)format;
#endif
}

/* 把环形缓冲全部刷到串口（panic/调试用） */
void klog_dump(void) {
    size_t total = klog_prod - klog_cons;
    if (total > KLOG_RING_SIZE) total = KLOG_RING_SIZE;
    size_t start = klog_prod - total;
    for (size_t i = 0; i < total; i++) {
        char c = klog_ring[(start + i) & (KLOG_RING_SIZE - 1)];
        if (stdio.handler) stdio.handler(&stdio, c);
    }
}
