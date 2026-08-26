#!/bin/bash
# gen_ksym.sh - 从 kernel.bin 生成包含 ksym 数组的 .o 文件，自动适配架构及工具链

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

# 检测架构（使用 file）
detect_arch() {
    local elf="$1"
    if ! command -v file >/dev/null 2>&1; then
        echo "Error: file command not found." >&2
        exit 1
    fi
    local file_output=$(file -b "$elf")
    if echo "$file_output" | grep -q "x86-64"; then
        echo "x86_64"
    elif echo "$file_output" | grep -q "RISC-V"; then
        echo "riscv64"
    else
        echo "Error: unsupported architecture, file output: $file_output" >&2
        exit 1
    fi
}

arch=$(detect_arch "$input")
echo "Detected architecture: $arch"

# 根据 TOOLCHAIN_PREFIX 环境变量设置 nm 和 gcc 命令
if [ -n "${TOOLCHAIN_PREFIX:-}" ]; then
    NM="${TOOLCHAIN_PREFIX}nm"
    GCC="${TOOLCHAIN_PREFIX}gcc"
    echo "Using toolchain prefix: $TOOLCHAIN_PREFIX"
else
    NM="nm"
    GCC="gcc"
    echo "Using plain nm/gcc (no prefix)"
fi

# 注意：不再使用 command -v 检查，直接信任环境。若命令不可用，后续步骤会自然失败

# 创建临时文件
tmp_sym=$(mktemp)
tmp_asm=$(mktemp --suffix=.s)

cleanup() {
    rm -f "$tmp_sym" "$tmp_asm"
}
trap cleanup EXIT

# 1. 获取符号表
"$NM" -n "$input" 2>/dev/null | awk '
    $2 !~ /^[UuWwVv]/ && $1 ~ /^[0-9a-fA-F]+$/ {
        name = substr($0, index($0, $3))
        print $1, name
    }
' > "$tmp_sym"

if [ ! -s "$tmp_sym" ]; then
    echo "Error: no valid symbols found or $NM failed." >&2
    exit 1
fi

# 2. 基址（可根据架构调整）
case "$arch" in
    x86_64)
        base=$((0xffffffff80000000))
        ;;
    riscv64)
        base=$((0xffffffc000000000))
        ;;
esac

# 3. 生成汇编
cat > "$tmp_asm" <<EOF
.section .st, "a", @progbits
.globl ksym_table_start
ksym_table_start:
EOF

i=0
str_defs=""
while read addr name; do
    offset=$((16#$addr - base))
    echo ".Lobj_${i}: .quad .Lstr_${i}" >> "$tmp_asm"
    echo ".quad ${offset}" >> "$tmp_asm"
    escaped_name=$(printf "%s" "$name" | sed 's/"/\\"/g; s/\\/\\\\/g')
    str_defs="${str_defs}.Lstr_${i}: .asciz \"${escaped_name}\"\n"
    i=$((i + 1))
done < "$tmp_sym"

cat >> "$tmp_asm" <<EOF
.globl ksym_table_end
ksym_table_end:
EOF

echo -e "$str_defs" >> "$tmp_asm"

# 4. 汇编
"$GCC" -c "$tmp_asm" -o "$output"

echo "Successfully generated $output with $i symbols for $arch"
