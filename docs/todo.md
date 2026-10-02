# TKernel2 待办事项（TODO）

> 最后更新：2026-09-18

## 已完成

### 本轮：交互式 busybox sh + console 串口输入 + 帧耗尽 Heisenbug 修复
- **交互式 sh + 串口输入打通**：
  - `console` 经 DPI 读回调支持 UART COM1 `inb` 轮询读取；内核
    `tty_read` 阻塞读（syscall 期间 IF=0，系统停等输入，符合预期）+ `\r→\n` +
    透传；`tty_write` 经 COM1 `outb` 输出。
  - `ioctl(0/1, TCGETS/TCSETS/TIOCGWINSZ/TIOCGPGRP/TIOCSPGRP)` 对 tty 返回成功
    （ash 据此进 lineedit 交互模式、打印 `/ # ` 提示符）；注意字段长度：
    `termios` 仅 60 字节（勿写 64）、`TIOCGPGRP` 参数是 `pid_t`（4 字节，勿写 8），
    越界会踩栈上相邻变量 → musl `hlt`(#GP) 崩溃。
  - 补齐 syscall 存根：`open`(-ENOENT)、`fcntl(F_SETFL)`、`writev`、
    `poll`（lineedit 等输入）、`getpgid`(=0)、`kill`(SIGTTIN 探测) ——
    否则 ash 进 job-control 死循环或 stderr 丢输出。
  - `init.c`：sh 由 `-c` 一次性冒烟改为常驻 `argv={"sh"}`；须等 devfs+ramdisk
    DPI 注册完、再留缓冲让 fsd 完成 mount，最后才 spawn sh（sh 阻塞在
    read(0) 时系统冻结等输入，故必须最后启动）。
- **根因：内核 heap 占满帧池 → 用户进程 spawn -12（Heisenbug）**：
  `UxTK/mem/heap.c` 的 `select_heap_space` 把 heap 设成**最大可用区全长**
  （qemu 报告可用内存 80~118MB 间浮动），`init_heap` 用
  `page_map_range_to_random` **逐 4KB 页 `alloc_frames(1)`** 占满整个帧池，
  后续 eloader 给用户进程建地址空间时 `alloc_frames` 失败（-12）→ ramdisk/fsd
  起不来、init 等 "ramdisk" DPI 注册死循环。修复：heap 大小**封顶 16MiB**
  （`KERNEL_HEAP_MIN_SIZE`，足够内核 malloc，留数万帧给用户进程）。
- **验证**（qemu + 串口 unix-socket 自动注入，连跑 3 次全 PASS，~19s/次）：
  `/ # ` 提示符出现 → 注入 `echo BUSYBOX-SH-OK` / `x=41; echo x=$x` 正确回显；
  同时跨地址空间 DPI 链路全 PASS（fsd `read_at(2584576) PASS`、跨空间写
  `/dev/console(29) PASS`）；**Panic=0 / page_fault=0**；clock tick 持续。

### 上轮：页表隔离 + eloader + busybox(sh) 打通（RIP=0 崩溃两连修）
- **修复 1 — eloader `map_range` 段数据错位**（`modules/eloader/eloader.c`）：
  `page_map_to` 丢弃 vaddr 低 12 位，而原实现从**非页对齐** p_vaddr 开始按
  `vaddr + i*0x1000` 建表且数据从**帧偏移 0** 拷贝 → 非对齐 RW 段（busybox
  `0x53a130`）整段错位 `vaddr & 0xfff` 字节：`.init_array[0]`（文件值 0x415e10）
  被写进帧偏移 0，用户在 0x53a130 读到 0 → musl `call *(%rbx)` 跳 0，
  `rip=0`、`[rsp]=0x4d6a96`。修复：按 `page_va = vaddr & ~0xfff` 建表，
  首页数据写帧内偏移 `off`，页数按 `off + p_memsz` 计算。自有 ELF（user.ld
  页对齐布局）不触发此 bug，故 devfs/fsd/ramdisk 一直正常。
- **修复 2 — `syscall_entry` 违反 Linux ABI**（`modules/linux_syscall/dispatch.c`）：
  装填 dispatch 参数后**从不恢复** RDI/RSI/RDX/R8/R9/R10 就 iretq 返回；
  Linux 只允许破坏 RAX/RCX/R11。busybox ash 跨 syscall 复用 R8，拿到内核
  装填的 a4=原 R10（=8），`mov 0x88(%r8)` 访问 [0x90] 缺页。定位手段：
  `qemu -d int -D log` 抓 #PF 全寄存器（RSI=1/R8=8 → 参数整体右移一位的
  指纹）。修复：入口把 6 个 caller-saved 压栈备份，返回逐一恢复；
  iretq 帧改用用户原 RFLAGS（R11），user_rsp 用首条 `pushq %rsp` 备份。
- **验证**（qemu10）：`BUSYBOX-SH-OK` + 变量展开 `x=41` + `done`，sh 正常
  exit；同时 devfs/ramdisk/fsd 隔离 DPI 链路全部 PASS（跨 CR3 read_at
  2580480 字节、backend 读写、跨空间写 /dev/console），clock tick 持续，
  **Panic=0 / page_fault=0**。

### 上轮：init 启动 user server + initrd 规范化 + devfs 挂载 /dev（含 console）
- **user server 改由 init 启动**：内核 `user` 模块**只**创建 init（pid1，root），
  不再内嵌/映射 fsd/ramdisk/devfs。新增扩展 syscall **`SYS_SPAWN`（0x7A0003）**：
  `syscall(SYS_SPAWN, name, rip, rsp, uid, gid) -> pid`，共享当前用户 CR3、内核
  分配 ring0 栈（`dispatch.c:spawn_user_process`）。
- **server 以 initrd 文件下发**：`/sbin/{devfs,ramdisk,fsd}`（ELF）。
  `init.c` 用户态：`tk_initrd_map` → 解析 cpio newc → 解析 ELF（PT_LOAD）→
  `mmap` 装载段与用户栈（rsp≡8 mod16）→ `tk_spawn` 启动。devfs/ramdisk 为
  root（需注册 DPI），**fsd 非 root uid=gid=1**（原 1000 改为 1）。
  `build_os.sh` 不再 gen_stub server，改为拷贝 ELF 到 `modules/sbin/`。
- **initrd 规范化**：内核模块从 initrd root 移到 **`etc/modules/*.tkm`**
  （`modules/Makefile`，cpio 路径）。**不改 `kernel/`**：现有后缀匹配对
  `etc/modules/X.tkm` 仍然命中（用户确认无需改）。
- **devfs 挂载 /dev + 内置 console**：devfs 注册 "devfs"（/dev 后端）并**默认
  注册 "console"**——read/write 回调在调用者 syscall 上下文（CPL0）直接做
  COM1 端口 I/O。fsd `mount /dev (devfs)` 读目录列表，并打开 /dev/console
  写入一行（经 console 回调 → COM1）。
- **验证**（`serial_devfs_console.log`，panic=0）：
  `Load module` 全部来自 etc/modules/；`init spawn {devfs,ramdisk,fsd} -> pid 2/3/4 OK`；
  devfs `/dev` 6 项（vty/null/udev/ramdisk/devfs/**console**）；
  `fsd: mount /dev (devfs handle=5)` 读到 6 项；
  `fsd: write /dev/console(29) -> 29 PASS`（串口出现该行）；/ 挂载与 DPI 链路仍 PASS。

### 前一轮：日志前缀规范化 + 用户态 devfs（DPI 枚举接口）
- **日志前缀**：去掉模块内手写的 `[linux_syscall]`/`[user]`（与 plogk 自动
  前缀重复），改用 plogk 的日志前缀栈 `plogk_info_stack`/`plogk_info_ptr`：
  - `linux_syscall`：新增 `LSC_LOGF(...)`（压 "LINUX_SYSCALL" → plogk → 弹栈），
    4 处运行时日志（mmap/brk/sys_exit/initrd_map）改写。
  - `user`：`module_init` 入口压入 "USER"、出口弹栈，删掉全部 `[user] ` 前缀。
  输出由 `[ Module ] [linux_syscall] mmap:` 变为 `[ LINUX_SYSCALL ] mmap:`。
- **DPI 用户态枚举接口**：新增 `DPI_OP_COUNT(9)`（返回驱动数）与
  `DPI_OP_ENUM(10)`（`a2=idx a3=buf a4=len` → 把第 idx 个驱动名拷入用户 buf，
  越界 `-ENOENT`）。供**用户态驱动**（devfs）拿到所有已注册驱动。
  （此前误做成内核 devfs 模块 + KPI，已按"devfs 必须是用户态驱动"撤销。）
- **用户态 devfs 驱动**（`~/tk-userland/src/devfs.c`，新 target `devfs`）：
  root 进程，经 `DPI_OP_REGISTER` 注册 "devfs"；主循环周期性用
  `tk_dpi_count`/`tk_dpi_enum` 重建 /dev 列表（后注册的 udev/ramdisk 也会
  出现，变化时重打印），`read_at` 返回列表、`ioctl(0x100)` 返回设备数。
  回调只从全局缓冲拷贝，不在回调里发 syscall（避免嵌套 syscall）。
- **user 模块**：新增 `DEVFS_BASE=0x0050b000`、stack6、ring0 stack6，创建 root
  任务 "devfs"。
- **fsd**：新增经 DPI 读取 /dev（`lookup("devfs")` + `read_at`）并打印。
- **验证**（`serial_devfs_test.log`，panic=0）：devfs 动态列出
  `vty/null/devfs` → `+udev` → `+ramdisk`；`fsd: /dev via devfs` 打印 4 个设备；
  ramdisk/fsd 链路仍全 PASS；日志前缀无重复。

### 前一轮：用户态 ramdisk 后端（DPI）+ fsd 改用 DPI 后端
- **DPI 扩展**：新增 op `DPI_OP_READ_AT(7)` / `DPI_OP_WRITE_AT(8)` 与回调
  `read_at/write_at(data, buf, len, off)`——块设备后端需要按偏移寻址。offset 经
  syscall 第 6 参（R9）传入。`dpi.h`（内核）与 `tkabi.h`（用户）结构布局同步
  （末尾追加 read_at/write_at，72 字节布局一致）。
- **用户态 ramdisk 服务器**（`tk-userland/src/ramdisk.c`，新 target `ramdisk`）：
  mmap 4MB 后端 RAM → 用 `tk_initrd_map` 取 initrd 镜像拷入后端 → 经
  `DPI_OP_REGISTER` 注册名为 **"ramdisk"** 的驱动（root 进程，handle=4），
  实现 `read_at/write_at` 与 `ioctl(GEOM=0x100)` 返回镜像长度。
- **fsd 改写**（`tk-userland/src/fs_server.c`）：不再直接 `sys_initrd_map`，
  改为经 DPI 查 "ramdisk" 句柄 → `ioctl` 取容量 → mmap 本地缓冲 →
  `read_at` 整体搬入镜像 → 按 newc 解析列出 / 条目；并做一次
  `write_at/read_at` 往返验证。保留 fsd（uid1000）注册被拒的负路径测试。
- **user 模块**：新增 `RAMDISK_BASE=0x00507000`、stack5、ring0 stack5，创建
  root 任务 "ramdisk"（在 fsd 之前创建，fsd 轮询等待驱动就绪）；ramdisk 二进制
  映射须带 `PTE_WRITEABLE`（要写全局 g_ram/g_size）。
- **修复两个内核潜藏 bug（本轮暴露）**：
  1. **用户栈初始 RSP 必须是 8 (mod 16)**（SysV ABI：函数入口 RSP≡8 mod 16）。
     原把 16 对齐的栈顶直接交给 stub，`struct tk_dpi` 变大后 GCC 改用 `movaps`
     清零局部结构体 → 未对齐 `#GP`。user.c 新增 `USER_ENTRY_RSP(top)=(top)-8`。
  2. **`syscall_entry` 栈参数压反**：`a6` 与 `user_rsp` 顺序错误，dispatch 收到
     a6=user_rsp、user_rsp=a6。此前无 syscall 用到第 6 参故未暴露；带偏移
     read_at 传入 off 变成用户 RSP。已改为先压 user_rsp、后压 a6（a6 落 `[rsp+8]`）。
- **验证**（`serial_ramdisk_test.log`，panic=0）：init ipcdemo/dpidemo 全 PASS；
  ramdisk 注册 handle=4；`fsd: ramdisk read_at(2560000) -> 2560000 PASS`；
  从后端列出 / 9 个条目（UxTK.tkm…sbin/init）、读 /UxTK.tkm 得 ELF 头；
  `backend write/read -> PASS`。

### 前一轮：task 模块用户/用户组 + DPI 注册限 root
- **task 模块**：`struct task` 末尾新增 `uid/gid`（**必须追加在末尾**——
  linux_syscall/user 下有 task.h 旧副本，只安全访问公共前缀字段，插中间会错位）。
  `task_create` 默认 `uid=gid=0`（root）：内核线程与 init 天然是 root；
  新增导出 `task_set_uidgid(struct task*, uid, gid)`（EXPORT_COUNT 17）供降权。
- **user 模块**：init 创建后显式 `task_set_uidgid(ut, 0, 0)`（防默认策略变化遗漏）；
  fsd 降权 `uid=gid=1000`。
- **dpi 模块**：`DPI_OP_REGISTER` 前置权限检查——`task_get_current()->uid != 0`
  返回 `-EPERM(-1)`；deps/Kconfig 增加 `TASK_MODULE`。
- **用户态**：`fs_server.c`（fsd，uid1000）启动时尝试注册驱动 → 实证
  `-1 DENIED (EPERM, PASS)`；init（root）注册 udev 仍成功。
- **验证**：`serial_dpi_test.log`——init uid=0 / fsd uid=1000 / fsd 注册被拒 /
  root 注册 udev 成功 / null 全 PASS / panic=0。

### 前轮：扩展 syscall 号段 + DPI 驱动接口模块
- **扩展 syscall 动态注册**（`modules/linux_syscall`）：其他模块可经 KPI 导出的
  `syscall_register(name, fn)` 注册附加 syscall，`syscall_id_by_name` /
  `syscall_name_by_id` 做 name↔id 转换。用户态 ABI 用 **0x7A0000 起的 TKernel2
  私有号段**（见 `modules/linux_syscall/include/syscall_reg.h`）：
  - `0x7A0000` 主分发：`syscall(0x7A0000, id, a1..a5) -> handler(a1..a5)`
  - `0x7A0001` name→id；`0x7A0002` id→name（拷入用户缓冲）
  - 号段选择依据：Linux syscall 号按架构独立分配，500~510 在 LoongArch 等表
    已分配（fanotify 系），MIPS64 用 5000~5999，ARM 私有区 0xf0000~0xf07ff；
    0x7A0000 与所有已知分配区间不重叠。
- **DPI 模块**（`modules/dpi`，DPI_MODULE，depends on LINUX_SYSCALL）：驱动注册
  `struct dpi { name, data, read, write, create, mmap, ioctl }`（成员可 NULL，
  用户态调用得 -errno；handle 非法 -EBADF，未实现 -ENOSYS）。向 linux_syscall
  注册一个 handler `"dpi"`（handler id=0），按 op 码二级路由：
  `DPI_OP_LOOKUP/READ/WRITE/CREATE/MMAP/IOCTL`（见 `modules/dpi/include/dpi.h`）。
  内置示例驱动 **vty**（COM1 虚拟终端，handle=1）与 **null**（read=0 EOF /
  write 丢弃返回 len / ioctl 忽略返回 0，handle=2）。KPI 导出 `dpi_register` /
  `dpi_lookup`。
- **用户态注册驱动（DPI_OP_REGISTER=6）**：ring3 传 `struct dpi` 指针经
  SYS_EXTRA 注册；内核拷贝结构体与驱动名（防篡改），回调指针保留用户地址，
  由 DPI 分发在调用者 syscall 上下文直接调用——依赖「全用户任务共享页目录」
  设计，per-task CR3 隔离后需改走 IPC 转发模型（复用现有 ipc 模块端口）。
  用户指针仅做规范地址粗检（与 dispatch 解引用用户缓冲同策略）。
- **用户态**（`~/tk-userland` 独立仓库）：`abi/tkabi.h` 增加 0x7A0000 号段常量、
  DPI op 码、`struct tk_dpi`、`tk_syscall5`（num+5 参，r10/r8/r9）与 `tk_dpi_*`
  包裹；`src/init.c` 增加 `[dpidemo]` 段（null write/ioctl/read、负路径 lookup
  -ENOENT、ring3 注册 udev 驱动并自测 ioctl 回显 cmd^data）。qemu 实证全部
  PASS、无 Panic。
- **修复 stub 单页映射**：init 二进制涨到 4579 字节超过 1 页，尾部 rodata 落入
  未映射区导致 #PF。`modules/user/user.c` 改为按 `user_stub_bin_len` 动态映射
  页数，stub 区预留 64KB；`dispatch.c` 的 `USER_BRK_BASE` 相应移到 0x410000。
- **加载顺序**（重新生成 manifest/list）：`... → linux_syscall → dpi → user`。
- **user 模块**：禁用 init2/init3 自旋测试（A/B 输出刷屏串口，抢占切换此前已
  验证），`user.c` 中 `#if 0` 包裹，init + fsd 正常。
- **验证**（`serial_dpi_test.log`，30s qemu）：LINUX_SYSCALL 注册 3 导出、
  `SYS_EXTRA=0x7a0000` 就绪、DPI_MODULE 注册 2 导出、vty handle=1、dpi handler
  id=0；init `[ring3] hello`、fsd 挂载 initrd、ipcdemo ALL OK，无 Panic。

### 本轮：ring3 + Linux ABI syscall
- **ring3 用户态进入 + Linux x86_64 ABI syscall 打通**：支持 `sys_read` / `sys_write` /
  `sys_brk` / `sys_mmap` / `sys_getpid` / `sys_exit`。`serial.log` 实证输出
  `[ring3] hello` / `brk OK` / `mmap OK`。
- **符号导出污染回退**：`kpi_export_sym.type` 字段及模块导出数组里的
  `STT_FUNC` / `STT_OBJECT` 第三元素（commit `6c06ee8` 引入，非用户所写）。已回退：
  - `include/tkm.h`：`kpi_export_sym` 恢复为 `name` + `value` 两字段。
  - `modules/UxTK/main.c`、`modules/task/module/task_module.c`、`modules/test/test.c`：
    导出数组恢复为两元素初始化。
  - `docs/module_development.md`、`readme.md`：导出符号示例代码同步改为两元素。

### 本轮：修复 `kernel.bin` 链接错误 `undefined reference to 'syscall_dispatch'`
- **根因**：顶层 `Makefile` 的 `S_SOURCES := $(shell find * -name "*.s")` 会递归扫到
  `modules/` 下的 `.s`（`syscall_entry.s` / `jump_to_user.s`），把它们链进 `kernel.bin`，
  但同模块的 `dispatch.c` 只编进 `.tkm`，不进内核链接 → `syscall_dispatch` 未定义。
- **修复**：`S_SOURCES` 改为只扫 `kernel boot lib`（与 `C_SOURCES` 范围一致）。
- **彻底消除**：两处 `.s` 已改为 C 内联汇编（见下），全树已无 `.s` 源文件。

### 本轮：`sys_exit` 正确销毁任务
- **根因 1（悬垂指针）**：`eevdf_find_eligible_task` 不从 ready_queue 摘除任务，运行中的
  任务仍在队列里；`eevdf_remove_task` 释放其 `task_info` 后**未清 `running_task`** →
  下个 tick 解引用已释放内存。
  修复：`eevdf_remove_task` 移除的若是 `running_task`，一并置 NULL。
- **根因 2（自死锁）**：`task_delete` 先 `LOCK(task_control_block.lock)`，再调用同样
  `LOCK` 同一把非递归自旋锁的 `eevdf_remove_task` → 自死锁（且 syscall 入口 `cli`，
  中断被屏蔽 → 永久挂死）。
  修复：把调度器摘除移到持锁区之外。
- 另：`sys_exit` 在 `task_exit` 后就地 idle，补 `sti` 让时钟中断继续；空闲提示只打一次。

### 本轮：`.s` 汇编 → C 内联汇编（naked）
- `modules/linux_syscall/syscall_entry.s` → `dispatch.c` 内 `__attribute__((naked))`
  的 `syscall_entry()`（基本内联汇编，指令与原 `.s` 一致）。
- `modules/task/core/jump_to_user.s` → `task.c` 内 `naked` 的 `jump_to_user_asm()`。
- 删除上述两个 `.s` 文件，并移除两个模块 Makefile 里已无用的 `%.o: %.s` 规则。

### 本轮：模块加载顺序（Kconfig 依赖 → 拓扑排序）
- **根因**：`gen_module_manifest.py` 提取依赖时误用 `parts[1]`（取到 `on`），导致所有
  模块依赖恒为空；且 Kconfig 用**配置符号名**（`TASK_MODULE`/`LINUX_SYSCALL`），而清单
  用**目录名**（`task`/`linux_syscall`），两者不匹配，拓扑排序认不出依赖。
  结果 `LINUX_SYSCALL` 排在 `TASK_MODULE` 之前加载，加载时 `task_exit` 解析成 0。
- **修复**：修正提取器为 `parts[2]`，并新增「配置符号 → 目录名」翻译；补
  `modules/linux_syscall/Kconfig` 的 `depends on TASK_MODULE`；重新生成清单与顺序。
- 现在的加载顺序：`UxTK → TEST → TEST2 → TASK_MODULE → LINUX_SYSCALL → USER`。

### 本轮：linker/模块架构统一 + 输出改 plogk
- **linker 接口统一为 `int elf_relocate_module(void *base, module_info *mod)`**（0 成功/-1 失败）：
  - `include/elf_parse.h` 声明由 `void` 改为 `int`；
  - `hal/arch/x86_64/linker/linker.c` 改为 `int`，不支持的重定位类型返回 -1；
  - `hal/arch/riscv64/linker.c` 恢复被注释的 `#include "elf_parse.h"`，与接口一致（本就是 `int`）。
- **移除 x86_64 linker 的调试输出**：删掉 `plogk_info_stack[...]="RELOC"` 前缀推入与
  `Unsupported type ...` 打印。
- **模块架构判断架构化**：`kernel/module/module.c` 新增 `module_arch_supported()` /
  `module_arch_name()`；**模块暂时先不支持 riscv**；两阶段重定位均检查返回值。
- **输出统一 plogk**：`kernel/module/module.c` 与全部 modules 的 `printk` → `plogk`，
  并去掉模块内手工 `ansi_V("[ Module ]")` 前缀（plogk 自动加前缀）。
  仍保留 `printk` 的仅输出子系统自身：`kernel/debug/printk.c`（实现）、
  `kernel/debug/debug.c`（panic/格式原语）、`kernel/kmain.c`（裸 ANSI 色条）。

### 本轮：用户态上下文保存/恢复（抢占式多进程切换）
- **关键约束**：`interrupt_frame_t` 只有 `rip/cs/rflags/rsp/ss`；`__attribute__((interrupt))`
  的 C handler 序言只把**调用方保存寄存器**压栈，**rbx/r12-r15 不在栈上**，无法靠
  "改写中断帧"在两个用户进程间交换 → 必须用裸汇编入口。
- **实现**（均在 `modules/task/`）：
  - `core/task.c` 新增 `__attribute__((naked))` 的 `task_timer_entry`：显式 push/pop
    **全部 15 个 GPR** + 复用 CPU 压入的控制帧；布局由 `struct user_cpu_ctx`（`include/task.h`）描述。
  - C 侧 `task_timer_dispatch(ctx)`：保存被打断的 ring3 上下文到 `current_task->uc`；
    时间片到（`TASK_PREEMPT_TICKS`）轮转下一个用户任务；把目标任务上下文写回 `ctx`
    由入口 `iretq` 恢复（首次进入走 `jump_to_user_asm`）。
  - per-task CR3（`mov %cr3`）与 per-task ring0 栈（写 TSS.rsp[0]）随每次切换更新。
  - 注册由 `task_timer_interrupt` 改为 `task_timer_entry`。
- **验证**（`user.c` 建 3 个用户进程：init=hello/brk/mmap/exit、init2/init3=自旋 A/B）：
  serial.log 实证 `helloB`/`helloA`/`[ring3] hello` 均出现，init2/init3 输出
  `B B A A A B B B A A A ...` **交替**（=抢占切换 + 上下文恢复生效），init 正常 exit 并
  `Deleted task init (ID: 1)`，全程 **Panic=0**。
  - 踩坑：`user_space_init` 建完 init2 后会被时钟抢占（`jump_to_user` 不返回）→ init3 建不出来。
    修复：本函数全程 `cli`，全部任务建好后再 `sti`。

## 待办

### 中优先级
- [ ] **busybox sh 增强**：目前只验证内建命令（echo/变量展开）；下一步让 ash
  能 fork+exec 外部 applet（需要 sys_fork/clone、wait4、execve 经 eloader
  装载新地址空间），以及 /dev/console 交互式读（getty 风格）。

### 低优先级 / 已知坑
- [ ] **跨模块读取"对象符号"拿到 0**：loader 对导出的对象符号处理有问题，故 `tss0`
  等以函数形式 `get_tss0_addr()` 导出。新增导出优先用函数形式。
- [ ] **导出符号统一为两元素** `{name, value}`，**禁止再加 `type` / `STT_*` 第三元素**
  —— 全树从未定义 `STT_FUNC` / `STT_OBJECT`，带第三元素会导致模块编译失败。
- [ ] **模块依赖排序的持久化**：`modules/Makefile` 只在清单/顺序文件**不存在**时才重新生成，
  文件存在但过期时不会刷新。改动依赖后需手动执行
  `python3 scripts/gen_module_manifest.py > modules/module_manifest.json` 与
  `python3 scripts/sort_modules.py > modules/module_list.txt`。

## 硬约束

- 未经用户明确授权，**禁止改写 `kernel/` `hal/` `include/`**，只允许改 `modules/` 与 `docs/`。
- 所有新建 markdown 文档统一放在 `docs/` 目录。
- 汇编优先用 **C 内联汇编（naked）**，不新增 `.s` 文件。
