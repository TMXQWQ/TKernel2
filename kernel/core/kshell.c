/*
 *debug用的shell
 */
#include "kshell.h"
#include "alloc.h"
#include "debug.h"
#include "hal.h"
#include "klibc.h"
#include "vfs.h"
#include "acpi.h"

vfs_inode_t *curdir;

buildin_cmd_t buildin_cmd[256] = {
    (buildin_cmd_t){.name = "mkdir", .len = 6, .func = mkdir},
    (buildin_cmd_t){.name = "touch", .len = 6, .func = touch},
    (buildin_cmd_t){.name = "ls", .len = 3, .func = ls},
    (buildin_cmd_t){.name = "cd", .len = 3, .func = cd},
};

int mkdir(char *cmd) {
  printk("cmd:%s\n", cmd);
  char *str = (char *)malloc(sizeof(char) * (strlen(cmd)));
  for (size_t i = 0; i < strlen(cmd) && cmd[i + 5] != '\0'; i++) {
    str[i] = cmd[i + 6];
  }
  printk("str:%s\n", str);
  if (str[0] == '/') {
    vfs_creat(str, VFS_DIR);
    return 0;
  }

  char *tmp = (char *)malloc(strlen(curdir->name) + strlen(str));
  if (curdir == &root) {
    strcpy(tmp, curdir->name);
    strcat(tmp, str);
    goto a;
  }

  strcpy(tmp, curdir->name);
  strcat(tmp, "/");
  strcat(tmp, str);
a:
  vfs_creat(tmp, VFS_FILE);
  free(tmp);
  return 0;
}

int touch(char *cmd) {
  printk("cmd:%s\n", cmd);
  char *str = (char *)malloc(sizeof(char) * (strlen(cmd)));
  for (size_t i = 0; i < strlen(cmd) && cmd[i + 5] != '\0'; i++) {
    str[i] = cmd[i + 6];
  }
  printk("str:%s\n", str);
  if (str[0] == '/') {
    vfs_creat(str, VFS_FILE);
    return 0;
  }

  char *tmp = (char *)malloc(strlen(curdir->name) + strlen(str));
  if (curdir == &root) {
    strcpy(tmp, curdir->name);
    strcat(tmp, str);
    goto a;
  }

  strcpy(tmp, curdir->name);
  strcat(tmp, "/");
  strcat(tmp, str);
a:
  vfs_creat(tmp, VFS_FILE);
  free(tmp);
  return 0;
}

int ls(char *cmd) {
  vfs_printk_ls(curdir);
  return 0;
}

int cd(char *cmd) {
  char *str =
      (char *)malloc(sizeof(char) * (strlen(cmd) + strlen(curdir->name)));
  for (size_t i = 0; i < strlen(cmd) && cmd[i + 2] != '\0'; i++) {
    str[i] = cmd[i + 3];
  }
  printk("cmd:%s\tstr:%s\tcurdir:%s\r\n", cmd, str, curdir->name);
  char *tmp;
  if (str[0] != '/') {
    tmp = strdup(curdir->name);
    strcat(tmp, str);
  } else {
    tmp = str;
  }

  if (vfs_basename(tmp, vfs_token_count(tmp)) == 0) {
    goto a;
  }

  if (strcmp(vfs_search(tmp)->name, vfs_basename(tmp, vfs_token_count(tmp)))) {
  a:
    printk("cd:无法访问 '%s': 没有那个文件或目录\r\n", tmp);
    return 0;
  }

  if (vfs_search(tmp)->type != VFS_DIR) {
    printk("cd: '%s': 不是目录\r\n", tmp);
    return 0;
  }
  curdir = vfs_search(tmp);

  return 0;
}

//主程序(由init创建的内核线程)
int kshell() {
  curdir = &root;
  for (;;) {
    printk("[\t%ld\t]\n", nano_time());
    continue;
    int count = 0;
    char cmd[256] = {'0'};
    printk("%s > ", curdir->name);
    for (count = 0; count < 255;) {
      char input = gets_serial();
      if (input == '\n' || input == '\r') {
        printk("\r\n");
        break;
      }
      if (input == '\b') {
        if (count == 0) {
          printk("\r%s > ", curdir->name);
          continue;
        }
        printk("\b \b");
        count--;
        continue;
      }
      cmd[count] = input;
      printk("%c", input);
      count++;
    }
    cmd[count + 1] = '\0';
    // printk("you press:\n%s\n",cmd);
    int flag = 0;
    for (uint64_t i = 0; i < NUM_OF_BUILDIN_CMDS; i++) {
      // printk("cmd %d:%s\n",i,buildin_cmd[i].name);
      if (!strncmp(cmd, buildin_cmd[i].name, strlen(buildin_cmd[i].name))) {
        buildin_cmd[i].func(cmd);
        flag = 1;
        break;
      }
    }
    if (flag == 1) {
      continue;
    }

    printk("Unkown command!\n");
  }
}
