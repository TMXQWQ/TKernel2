#include <stddef.h>
#include <stdint.h>

typedef struct __attribute__((packed)) {
        uint8_t c_magic[6]; // 魔数："070701"(newc)或"070702"(crc)
        uint8_t c_ino[8];   // inode号，16进制ASCII
        //          --------------------ignore---------------------
        uint8_t c_mode[8];  // 文件模式和类型，16进制ASCII  ignore
        uint8_t c_uid[8];   // 用户ID，16进制ASCII  ignore
        uint8_t c_gid[8];   // 组ID，16进制ASCII    ignore
        uint8_t c_nlink[8]; // 硬链接数，16进制ASCII    ignore
        uint8_t c_mtime[8]; // 修改时间，16进制ASCII    ignore
        //          --------------------------------------------------
        uint8_t c_filesize[8]; // 文件大小，16进制ASCII
        //          --------------------ignore---------------------
        uint8_t c_devmajor[8];  // 设备主号，16进制ASCII    ignore
        uint8_t c_devminor[8];  // 设备次号，16进制ASCII    ignore
        uint8_t c_rdevmajor[8]; // 特殊文件设备主号 ignore
        uint8_t c_rdevminor[8]; // 特殊文件设备次号 ignore

        uint8_t c_namesize[8]; // 文件名长度（含\0），16进制ASCII

        uint8_t c_check[8]; // 校验和（newc为0，crc格式使用）   ignore
        uint8_t c_name[];
} newc_header;

typedef struct {
        uint64_t  c_ino; // inode号
        size_t    c_filesize;
        uintptr_t data_ptr;
        char     *name;
} newc_file;

typedef struct {
        size_t     size;
        newc_file *file_list;
} newc_filesystem;

newc_filesystem cpio_parse(newc_header *base);
