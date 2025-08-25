#include "ramfs.h"
#include "alloc.h"

vfs_file_operations ramfs_ops = {
    .mnt = ramfs_mnt,
    .umnt = ramfs_umnt,
    .open = ramfs_open,
    .read = ramfs_read,
    .write = ramfs_write,
    .load = ramfs_load,
    .sync = ramfs_sync,
    .close = ramfs_close,
    .creat = ramfs_creat,
};

int ramfs_mnt(vfs_inode_t *inode) {
  inode->i_private = malloc(sizeof(ramfs_inode));
  ramfs_inode *ri_ptr = (ramfs_inode *)inode->i_private;
  ri_ptr->blocks = NULL;
  // ri_ptr->count = 0;
}
int ramfs_umnt() {}

int ramfs_open(vfs_inode_t *inode) { return 0; }

//注:size以1B(8bit)为单位
ssize_t ramfs_read(vfs_inode_t *inode, void *buf, size_t size, off_t offset) {
  ramfs_inode *ramfs_i = (ramfs_inode *)inode->i_private;
  ramfs_block *ramfs_b = ramfs_i->blocks;
  if (offset > inode->i_size) {
    return 0;
  }
  if (buf == NULL) {
    cpu_hlt;
  }

  ssize_t s;
  memcpy(buf, ramfs_b->addr + offset, size);
  return size;
}

ssize_t ramfs_write(vfs_inode_t *inode, const void *buf, size_t size,
                    off_t offset) {
  ramfs_inode *ramfs_i = (ramfs_inode *)inode->i_private;
  ramfs_block *ramfs_b = ramfs_i->blocks;
  if (size == 0) {
    return 0;
  }

  if (ramfs_b == NULL) //空文件
  {
    inode->i_size = size + offset;
    ramfs_b = (ramfs_block *)malloc(sizeof(ramfs_block));
    ramfs_b->size = size + offset;
    ramfs_b->addr = malloc(size + offset);
    memset(ramfs_b->addr, 0, offset);
    ramfs_i->blocks = ramfs_b;
  } else if (size + offset > ramfs_b->size) {
    ramfs_b->addr = realloc(ramfs_b->addr, size + offset);
  }
  ssize_t s;
  memcpy(ramfs_b->addr + offset, buf, size);
  return size;
}
void *ramfs_load(vfs_inode_t *inode) {}
int ramfs_sync(vfs_inode_t *inode) {}
int ramfs_close(vfs_inode_t *inode) {}

int ramfs_creat(vfs_inode_t *inode) {
  inode->i_private = malloc(sizeof(ramfs_inode));
  ramfs_inode *ri = (ramfs_inode *)inode->i_private;
  ri->blocks = NULL;
  return 0;
}
