#include "serial.h"
#include "common.h"
#include "kernel.h"
#include "limine.h"
#include "printk.h"
#include "stdint.h"

static uint8_t serial_calculate_lcr(void)
{
    uint8_t lcr = 0;

    switch (SERIAL_DATA_BITS) {
        case 5 :
            lcr |= 0x00;
            break;
        case 6 :
            lcr |= 0x01;
            break;
        case 7 :
            lcr |= 0x02;
            break;
        case 8 :
            lcr |= 0x03;
            break;
        default :
            lcr |= 0x03;
    }

    if (SERIAL_STOP_BITS == 2) lcr |= 0x04;

    switch (SERIAL_PARITY) {
        case 0 : // No parity
            lcr |= 0x00;
            break;
        case 1 : // Odd parity
            lcr |= 0x08;
            break;
        case 2 : // Even parity
            lcr |= 0x18;
            break;
        case 3 : // Mark parity
            lcr |= 0x28;
            break;
        case 4 : // Space parity
            lcr |= 0x38;
            break;
        default :
            lcr |= 0x00;
    }
    return lcr;
}

static void uart_write_reg(int reg, uint8_t val)
{
    uint64_t       hhdm_offset = hhdm_request.response->offset;
    volatile void *serial_base = (volatile void *)(hhdm_offset + 0x10000000);
    mmio_write8(serial_base + reg, val);
}
static uint8_t uart_read_reg(int reg)
{
    uint64_t       hhdm_offset = hhdm_request.response->offset;
    volatile void *serial_base = (volatile void *)(hhdm_offset + 0x10000000);
    return mmio_read8(serial_base + reg);
}

void init_serial(void)
{
    uint64_t       hhdm_offset = hhdm_request.response->offset;
    volatile void *serial_base = (volatile void *)(hhdm_offset + 0x10000000);
    // 关中断
    uart_write_reg(SERIAL_REG_IER, 0x00);
    // DLAB=1 设置波特率
    uart_write_reg(SERIAL_REG_LCR, 0x80);
    uint32_t divisor = 100000000 / (16 * SERIAL_BAUD_RATE);
    uart_write_reg(SERIAL_REG_DATA, divisor & 0xFF);
    uart_write_reg(SERIAL_REG_IER, (divisor >> 8) & 0xFF);
    // 设置数据格式
    uart_write_reg(SERIAL_REG_LCR, serial_calculate_lcr()); // 可复用原函数，需移到头文件
    // 启用 FIFO
    uart_write_reg(SERIAL_REG_FCR, 0xCF);
    // 设置 MODEM 控制
    uart_write_reg(SERIAL_REG_MCR, 0x0F);
    mmio_write8(serial_base + SERIAL_REG_DATA, '\n');

    // 注册到 stdio
    stdio.data    = (void *)serial_base;
    stdio.handler = serial_handle;
}

void write_serial(uintptr_t base, uint8_t data)
{
    while (!(uart_read_reg(SERIAL_REG_LSR) & 0x20));
    uart_write_reg(SERIAL_REG_DATA, data);
}

uint8_t serial_handle(struct writer *writer, char ch)
{
    write_serial((uintptr_t)(writer->data), ch);
    return 0;
}