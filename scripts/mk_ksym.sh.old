#!/bin/bash
# 用法：./mk_ksym.sh kernel.bin ksym.o

set -e
set -u

if [ $# -ne 2 ]; then
    echo "Usage: $0 <kernel.bin> <output.o>"
    exit 1
fi

input="$1"
output="$2"

if [ ! -f "$input" ]; then
    echo "Error: input file '$input' not found."
    exit 1
fi

tmp_sym=$(mktemp)
tmp_asm=$(mktemp --suffix=.s)

cleanup() {
    rm -f "$tmp_sym" "$tmp_asm"
}
trap cleanup EXIT

# 1. 用 nm 获取符号表，过滤未定义/弱符号，输出地址和名称
nm -n "$input" 2>/dev/null | awk '
    $2 !~ /^[UuWwVv]/ && $1 ~ /^[0-9a-fA-F]+$/ {
        name = substr($0, index($0, $3))
        print $1, name
    }
' > "$tmp_sym"

if [ ! -s "$tmp_sym" ]; then
    echo "Error: no valid symbols found or nm failed."
    exit 1
fi

# 2. 固定基址（内核虚拟起始地址）
base=$((0x0))

# 3. 生成汇编文件
cat > "$tmp_asm" <<EOF
.section .st, "a", @progbits
EOF

i=0
str_defs=""
while read addr name; do
    # 计算相对偏移（符号地址 - 基址）
    offset=$((16#$addr - base))
    echo ".Lobj_${i}: .quad .Lstr_${i}" >> "$tmp_asm"
    echo ".quad ${offset}" >> "$tmp_asm"
    str_defs="${str_defs}.Lstr_${i}: .asciz \"${name}\"\n"
    i=$((i + 1))
done < "$tmp_sym"

echo -e "$str_defs" >> "$tmp_asm"

# 4. 汇编生成目标文件
gcc -c "$tmp_asm" -o "$output"
# 也可使用 as --64 -o "$output" "$tmp_asm"

echo "Successfully generated $output"