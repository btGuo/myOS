#include "inode.h"

#include "local.h"
#include "inode.h"

struct fext_inode_m *file_create(const char *path);
int32_t inode_write(struct fext_inode_m *inode, const void *buf, uint32_t count);
