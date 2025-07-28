#include "vfs.h"
#include "alloc.h"
#include "debug.h"
#include "klibc.h"
#include "ramfs.h"

list_t *ptr_list; //句柄
vfs_inode_t root; //暂时先放这

int vfs_init() {
  root = (vfs_inode_t){.id = 0,
                       .i_private = NULL,
                       .i_size = 0,
                       .name = "/",
                       .type = VFS_DIR,
                       .file_list = (list_t){
                           .next = NULL,
                           .pre = NULL,
                           .data = NULL,
                       }};
  vfs_inode_t *dot, *dotdot;
  dot = (vfs_inode_t *)malloc(sizeof(vfs_inode_t));
  dotdot = (vfs_inode_t *)malloc(sizeof(vfs_inode_t));
  dot->file_list = root.file_list;
  dot->name = ".";
  dot->type = VFS_LINK;
  dot->i_private = (void *)&root;

  dotdot->file_list = root.file_list;
  dotdot->name = "..";
  dotdot->type = VFS_LINK;
  dotdot->i_private = (void *)&root;
  root.file_list = (list_t){
      .next = &(root.file_list),
      .pre = &(root.file_list),
      .data = NULL,
  };
  vfs_mount(
      (vfs_filesystem_t){
          .name = "root", .id = &root, .ops = ramfs_ops, .fs_private = NULL},
      &root);
  return 0;
}

int vfs_mount(vfs_filesystem_t fs, vfs_inode_t *inode) {
  // fs_list->next = (list_t*)malloc(sizeof(list_t));
  // fs_list->next->pre = fs_list;
  // fs_list->next->next = fs_list;
  // fs_list->pre = fs_list->next;
  // fs_list->next->data = (void*)malloc(sizeof(vfs_filesystem_t));
  // strcpy(((vfs_filesystem_t*)(fs_list->next->data))->name,fs.name);
  // ((vfs_filesystem_t*)(fs_list->next->data))->id=inode;
  // ((vfs_filesystem_t*)(fs_list->next->data))->ops=fs.ops;
  // ((vfs_filesystem_t*)(fs_list->next->data))->fs_private=fs.fs_private;
  if (inode->type != VFS_DIR) {
    return -EETYPE;
  }

  inode->i_size = 0;
  inode->i_mnt = (vfs_filesystem_t *)malloc(sizeof(vfs_filesystem_t));
  inode->i_mnt->id = inode;
  inode->i_mnt->name = (const char *)malloc(sizeof(char) * strlen(fs.name) + 1);
  strcpy(inode->i_mnt->name, fs.name);
  inode->i_mnt->ops = fs.ops;
  inode->i_mnt->fs_private = fs.fs_private;
  fs.ops.mnt(&root);
  return 0;
}

int vfs_unmount(vfs_filesystem_t fs) {} //懒得实现了,以后再说(

static inline char *pathtok(char **sp) {
  char *s = *sp, *e = *sp;
  if (*s == '\0')
    return NULL;
  for (; *e != '\0' && *e != '/'; e++) {
  }
  *sp = (*e == '/') ? e + 1 : e; // 保留 '/' 后的字符
  if (*e != '\0')
    *e = '\0'; // 仅当非结尾时替换为 \0
  return s;
}

//返回inode,仅支持绝对路径
// vfs_inode_t vfs_open(const char* path) {
//     // 检查路径有效性
//     if (!path || path[0] != '/') {
//         return (vfs_inode_t){0};  // 仅支持绝对路径
//     }

//     // 直接处理根路径 "/"
//     if (strcmp(path, "/") == 0) {
//         return root;  // 返回根目录 inode
//     }

//     // 复制路径（因 pathtok 会修改字符串）
//     char* path_copy = strdup(path);
//     if (!path_copy) return (vfs_inode_t){0};

//     char* state = path_copy;
//     vfs_inode_t* current = &root;

//     // 跳过根目录的 '/'（第一个 token 是空字符串）
//     char* token = pathtok(&state);
//     if (!token) {
//         free(path_copy);
//         return *current;  // 路径仅为 "/"
//     }

//     // 逐级解析路径
//     while (token && current) {
//         if (!current->file_list) {  // 检查目录是否可读
//             free(path_copy);
//             return (vfs_inode_t){0};
//         }

//         int found = 0;
//         for (int i = 0; i < 256 && current->file_list[i].name; i++) {
//             if (strcmp(current->file_list[i].name, token) == 0) {
//                 current = &current->file_list[i];
//                 found = 1;
//                 break;
//             }
//         }

//         if (!found) {
//             free(path_copy);
//             return (vfs_inode_t){0};  // 路径不存在
//         }
//         token = pathtok(&state);
//     }

//     free(path_copy);
//     return *current;
// }

vfs_inode_t *vfs_open(const char *path) {
  vfs_inode_t *inode = vfs_search(path);
  if (!inode)
    return NULL;

  // 调用文件系统的 open 操作（如果存在）
  if (inode->i_mnt && inode->i_mnt->ops.open) {
    if (inode->i_mnt->ops.open(inode) != 0) {
      return NULL;
    }
  }
  return inode;
}

// 文件读取函数
ssize_t vfs_read(vfs_inode_t *inode, void *buf, size_t size, off_t offset) {
  // 验证输入参数
  if (inode == NULL || buf == NULL || size == 0) {
    return -EINVAL; // 无效参数
  }

  // 检查文件类型
  if (inode->type != VFS_FILE) {
    return -EETYPE; // 是目录不是文件
  }

  // 检查文件系统操作是否可用
  if (inode->i_mnt == NULL || inode->i_mnt->ops.read == NULL) {
    return -ENOSYS; // 操作未实现
  }

  if (inode->i_size = 0) {
    return 0; // EOF
  }

  // 调用文件系统特定的读取操作
  return inode->i_mnt->ops.read(inode, buf, size, offset);
}

// 文件写入函数
ssize_t vfs_write(vfs_inode_t *inode, const void *buf, size_t size,
                  off_t offset) {
  // 验证输入参数
  if (inode == NULL || buf == NULL || size == 0) {
    return -EINVAL;
  }

  // 检查文件类型
  if (inode->type != VFS_FILE) {
    return -EETYPE;
  }

  // 检查文件系统操作是否可用
  if (inode->i_mnt == NULL || inode->i_mnt->ops.write == NULL) {
    return -ENOSYS;
  }

  // 调用文件系统特定的写入操作
  return inode->i_mnt->ops.write(inode, buf, size, offset);
}

// 文件关闭函数
int vfs_close(vfs_inode_t *inode) {
  if (inode == NULL) {
    return -EBADF; // 错误的文件描述符
  }

  // 检查文件系统操作是否可用
  if (inode->i_mnt != NULL && inode->i_mnt->ops.close != NULL) {
    // 调用文件系统特定的关闭操作
    return inode->i_mnt->ops.close(inode);
  }

  // 如果没有特定的关闭操作，返回成功
  return 0;
}

// 文件同步函数
int vfs_sync(vfs_inode_t *inode) {
  if (inode == NULL) {
    return -EBADF;
  }

  // 检查文件系统操作是否可用
  if (inode->i_mnt != NULL && inode->i_mnt->ops.sync != NULL) {
    return inode->i_mnt->ops.sync(inode);
  }

  // 如果没有同步操作，返回成功
  return 0;
}

// 文件加载到内存
void *vfs_load(vfs_inode_t *inode) {
  if (inode == NULL) {
    return NULL;
  }

  // 检查文件系统操作是否可用
  if (inode->i_mnt != NULL && inode->i_mnt->ops.load != NULL) {
    return inode->i_mnt->ops.load(inode);
  }

  // 如果没有加载操作，返回NULL
  return NULL;
}

//句柄转inode
vfs_inode_t vfs_get_inode(int ptr) {}

/**
 * 创建新文件
 * @param path  文件绝对路径（必须包含文件名）
 * @return      成功返回新建文件的inode指针，失败返回NULL
 */
vfs_inode_t *vfs_creat(const char *path, int fs_flag) {
  printk("path:%s\n", path);
  // 1. 验证路径有效性
  if (!path || path[0] != '/') {
    return NULL; // 仅支持绝对路径
  }

  // 2. 分离目录路径和文件名
  int token_count = vfs_token_count(path);
  if (token_count < 1) {
    return NULL; // 无效路径
  }

  char *filename = vfs_basename(path, token_count - 1); // 获取文件名
  printk("filename:%s\n", filename);
  if (!filename) {
    return NULL;
  }

  // 3. 获取父目录inode
  char *parent_path = strdup(path);
  if (!parent_path) {
    free(filename);
    return NULL;
  }

  // 截断路径到父目录（移除最后的文件名）
  char *last_slash = strrchr(parent_path, '/');
  if (last_slash) {
    *last_slash = '\0';
    if (strlen(parent_path) == 0) {
      strcpy(parent_path, "/"); // 处理根目录情况
    }
  }
  vfs_inode_t *parent_dir = vfs_search(parent_path);
  free(parent_path);
  if (parent_dir == NULL) {
    return NULL;
  }

  // 4. 验证父目录
  if (!parent_dir || parent_dir->type != VFS_DIR) {
    free(filename);
    return NULL; // 父目录不存在或不是目录
  }
  // printks("debug1\n");
  // 5. 检查文件是否已存在
  list_t *pos = parent_dir->file_list.next;
  int flag = 0;
  if (pos->data == NULL) //空目录
  {
    flag = 1;
  }
  while (pos->next != &parent_dir->file_list) {
    vfs_inode_t *entry = (vfs_inode_t *)pos->data;
    if (entry && entry->name && strcmp(entry->name, filename) == 0) {
      free(filename);
      return NULL; // 文件已存在
    }
    pos = pos->next;
  }
  // printks("debug2\n");
  // 6. 创建新inode
  vfs_inode_t *new_inode = (vfs_inode_t *)malloc(sizeof(vfs_inode_t));
  if (!new_inode) {
    free(filename);
    return NULL;
  }

  // 初始化inode
  memset(new_inode, 0, sizeof(vfs_inode_t));
  new_inode->name = filename;
  new_inode->type = fs_flag;
  new_inode->i_mnt = parent_dir->i_mnt;
  INIT_LIST_HEAD(&new_inode->file_list); // 初始化文件链表
  if (flag == VFS_DIR) {
    new_inode->file_list.next = (list_t *)malloc(sizeof(list_t));
    new_inode->file_list.data = (void *)malloc(sizeof(vfs_inode_t));
    new_inode->file_list.next->next = &(new_inode->file_list);
    new_inode->file_list.next->pre = &(new_inode->file_list);
    new_inode->file_list.next->data = (void *)malloc(sizeof(vfs_inode_t));
    vfs_inode_t *in = (vfs_inode_t *)new_inode->file_list.data;
    in->file_list = new_inode->file_list;
    in->name = ".";
    in->type = VFS_LINK;
    in->i_private = (void *)new_inode;
    in = (vfs_inode_t *)new_inode->file_list.next->data;
    in->file_list = new_inode->file_list;
    in->name = "..";
    in->type = VFS_LINK;
    in->i_private = (void *)parent_dir;
  }

  list_t *new_node;
  // 7. 添加到父目录
  new_node = (list_t *)malloc(sizeof(list_t));
  if (!new_node) {
    free(new_inode);
    free(filename);
    return NULL;
  }
  if (flag == 0) {
    new_node->data = new_inode;
    parent_dir->file_list.pre = new_node;
    pos->next = new_node;
    new_node->next = &parent_dir->file_list;
  } else {
    new_node->data = new_inode;
    parent_dir->file_list.data = new_node->data;
  }

  // 8. 调用文件系统的creat操作（如果存在）
  if (parent_dir->i_mnt && parent_dir->i_mnt->ops.creat) {
    if (parent_dir->i_mnt->ops.creat(new_inode) != 0) {
      list_del(new_node);
      free(new_node);
      free(new_inode);
      return NULL;
    }
  }

  return new_inode;
}

/**
 * 根据路径查找对应的 inode
 * @param path  绝对路径（必须以'/'开头）
 * @return      成功返回 inode 指针
 *              若路径不存在，返回路径中最后一个存在的项
 *              完全无效路径返回 NULL
 */
vfs_inode_t *vfs_search(const char *path) {
  if (!path || path[0] != '/') {
    return NULL; // 仅支持绝对路径
  }

  // 直接返回根目录
  if (strcmp(path, "/") == 0) {
    return &root;
  }

  char *path_copy = strdup(path);
  if (!path_copy)
    return NULL;

  char *state = path_copy;
  vfs_inode_t *current = &root;
  vfs_inode_t *last_valid = &root; // 始终记录最后一个有效节点

  // 跳过根目录的'/'
  char *token = pathtok(&state);
  if (!token) {
    free(path_copy);
    return current; // 路径为 "/"
  }

  // 逐级搜索路径
  while (token && current) {
    last_valid = current; // 更新最后一个有效节点

    // 如果当前不是目录，无法继续搜索
    if (current->type != VFS_DIR) {
      free(path_copy);
      return last_valid;
    }

    // 遍历链表查找匹配的token
    int found = 0;
    list_t *pos = current->file_list.next; // 从第一个节点开始

    while (pos != &current->file_list) { // 循环直到回到头节点
      vfs_inode_t *entry = (vfs_inode_t *)pos->data;

      if (entry && entry->name && strcmp(entry->name, token) == 0) {
        current = entry;
        last_valid = current;
        found = 1;
        break;
      }
      pos = pos->next;
    }

    // 未找到当前层级的 token
    if (!found) {
      free(path_copy);
      return last_valid;
    }

    token = pathtok(&state); // 获取下一级token
  }

  free(path_copy);
  return current;
}

/**
 * 根据路径查找对应的 inode
 * @param path  绝对路径（必须以'/'开头）
 * @return      成功返回 inode 指针
 *              若路径不存在，返回NULL
 */
vfs_inode_t *_vfs_search(const char *path) {
  // 0. 参数检查
  if (!path || path[0] != '/') {
    return NULL;
  }

  // 1. 处理根目录特殊情况
  if (strcmp(path, "/") == 0) {
    return &root; // 修改：根目录应返回root而非NULL
  }

  // 2. 复制路径
  char *path_copy = strdup(path);
  if (!path_copy)
    return NULL;

  // 3. 初始化搜索状态
  char *state = path_copy;
  vfs_inode_t *current = &root;
  vfs_inode_t *result = NULL;

  // 4. 跳过根目录的'/'
  char *token = pathtok(&state);
  if (!token) {
    goto cleanup; // 路径为"/"已处理，这里应该是无效情况
  }

  // 5. 逐级搜索路径
  while (token && current) {
    // 检查当前节点是否为目录
    if (current->type != VFS_DIR) {
      goto cleanup;
    }

    // 遍历链表查找匹配的token
    int found = 0;
    list_t *pos = current->file_list.next;
    list_t *head = &current->file_list;

    while (pos != head) {
      vfs_inode_t *entry = (vfs_inode_t *)pos->data;

      // 安全检查entry和entry->name
      if (entry && entry->name && strcmp(entry->name, token) == 0) {
        current = entry;
        found = 1;
        break;
      }
      pos = pos->next;
    }

    if (!found) {
      goto cleanup;
    }

    token = pathtok(&state);
  }

  // 6. 搜索成功
  result = current;

cleanup:
  // 7. 确保path_copy有效后再释放
  if (path_copy) {
    free(path_copy);
    path_copy = NULL;
  }

  return result;
}
/**
 * 获取路径中指定位置的 token
 * @param path  文件路径
 * @param n     要获取的 token 位置（从0开始）
 * @return      新分配的字符串包含第n个 token
 *              调用者需负责释放内存
 *              如果n超出范围返回 NULL
 */
char *vfs_basename(const char *path, int n) {
  if (!path || n < 0)
    return NULL;

  char *path_copy = strdup(path);
  if (!path_copy)
    return NULL;

  char *state = path_copy;
  char *token = NULL;
  char *result = NULL;
  int current = 0;

  // 特殊处理根路径
  if (strcmp(path, "/") == 0) {
    free(path_copy);
    return (n == 0) ? strdup("/") : NULL;
  }

  while (token = pathtok(&state)) {
    if (*token != '\0') { // 跳过空token
      if (current == n) {
        result = strdup(token);
        break;
      }
      current++;
    }
  }

  free(path_copy);
  return result;
}

/**
 * 计算路径中的token数量
 * @param path  文件路径
 * @return      token总数（不包括空token）
 */
int vfs_token_count(const char *path) {
  if (!path)
    return 0;

  char *ptr, *path_copy = strdup(path);
  if (!path_copy)
    return 0;
  ptr = path_copy;

  char *state = path_copy;
  char *token = NULL;
  int count = 0;

  while ((token = pathtok(&state))) {
    if (*token != '\0')
      count++;
  }
  free(ptr);
  return count;
}

list_t *vfs_printk_ls(vfs_inode_t *i) {
  if (i->type != VFS_DIR) {
    return -EETYPE;
  }
  list_t *ls = &(i->file_list);
  // printk("%p ,%p ", ls, ls->next);
  printk("%s ", ((vfs_inode_t *)(ls->data))->name);
  for (; ls->next != &(i->file_list);) {
    ls = ls->next;
    // printk("%p ,%p ", ls, ls->next);
    printk("%s ", ((vfs_inode_t *)ls->data)->name);
  }
  printk("\n");
}
