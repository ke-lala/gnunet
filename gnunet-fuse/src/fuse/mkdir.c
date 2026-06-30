/*
 * mkdir.c - FUSE mkdir function
 *
 *  Created on: Mar 14, 2012
 *  Author: mg
 *
 *
 * 	Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>


int
gn_mkdir (const char *path, mode_t mode)
{
  return 0;
}
