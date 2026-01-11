#include "cpio.h"
#include "stdint.h"
#include "string.h"

newc_filesystem ncfs;
newc_file       file_list[128];

static inline uint32_t hex_char_to_value(char c)
{
    // 使用查找表（编译时常量）
    static const uint8_t hex_table[256] = {
        ['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,  ['4'] = 4,  ['5'] = 5,  ['6'] = 6,  ['7'] = 7,  ['8'] = 8,  ['9'] = 9,  ['A'] = 10,
        ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15, ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
    };
    return hex_table[(uint8_t)c];
}

static inline uint32_t cpio_hex8_to_int_manual(const char hex[8])
{
    uint32_t result = 0;

    // 展开循环以优化性能
    result = (hex_char_to_value(hex[0]) << 28) | (hex_char_to_value(hex[1]) << 24) | (hex_char_to_value(hex[2]) << 20)
             | (hex_char_to_value(hex[3]) << 16) | (hex_char_to_value(hex[4]) << 12) | (hex_char_to_value(hex[5]) << 8)
             | (hex_char_to_value(hex[6]) << 4) | (hex_char_to_value(hex[7]) << 0);

    return result;
}

newc_filesystem cpio_parse(newc_header *base)
{
    newc_header *header = base;
#define tmp(a, b)   a->b[0], a->b[1], a->b[2], a->b[3], a->b[4], a->b[5], a->b[6], a->b[7]
#define align(addr) ((addr + 3) & ~3)
    for (int i = 0; i < 128; i++) {
        uint64_t ino, size, namesize;
        ino      = cpio_hex8_to_int_manual((char[8]) {tmp(header, c_ino)});
        size     = cpio_hex8_to_int_manual((char[8]) {tmp(header, c_filesize)});
        namesize = cpio_hex8_to_int_manual((char[8]) {tmp(header, c_namesize)});
        if (namesize >= 11 && !strcmp((char *)header->c_name, "TRAILER!!!")) {
            size = i;
            break;
        }
        if (i == 127) {
            size = i;
            break;
        }
        file_list[i] = (newc_file) {ino, size, (align(((uintptr_t)header + 110 + namesize))), (char*)header->c_name};
        header       = (newc_header *)(align((uintptr_t)(((uintptr_t)header) + 110 + namesize + size)));
    }
#undef tmp   //(a, b)
#undef align //(addr)
    ncfs.file_list = file_list;
    return ncfs;
}
