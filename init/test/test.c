int module_enter(void* func)
{
    ((void(*)(const char *format, ...))func)("test is running!!!\n");
    return 114514;
}