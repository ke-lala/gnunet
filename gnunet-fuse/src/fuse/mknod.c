/*
 * mknod.c - FUSE mknod function
 *
 *  Created on: Mar 14, 2012
 *      Author: mg
 *
 *
 * Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>

int
gn_mknod (const char *path, mode_t mode, dev_t rdev)
{
  return 0;
}
