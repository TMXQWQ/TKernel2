#pragma once

#define NUM_OF_BUILDIN_CMDS (4)

int kshell();

typedef struct 
{
    char* name;
    int len;
    int (*func)(char* cmd);
}buildin_cmd_t;

int mkdir(char* cmd);
int touch(char* cmd);
int ls(char* cmd);
int cd(char* cmd);