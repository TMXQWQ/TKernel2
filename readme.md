# TKernel

#### 介绍
TKernel:一个微内核+的实现

#### 结构层次

TKernel 是完全自研不依赖于任何项目的内核

其层次（由低到高）为

```

                                            ——VFS（Rootfs）(in boot.img)
                                          /
硬件抽象层(HAL)——kernel——Kpi（in boot.img）{  ——kernel modules(in boot.img and run as system thread)
                                          \
                                            ——module manager(in boot.img)——Mpi(in boot.img)——system modules(run as root)

```
其中，system module（系统模块）是在[base os](https://gitee.com/space_Tkj_TMX_space/base-os)中实现的。
