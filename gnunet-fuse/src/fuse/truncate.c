/*
 * truncate.c - FUSE truncate function
 *
 *  Created on: Mar 14, 2012
 *      Author: mg
 *
 * Change the size of a file
 *
 * */

#include <errno.h>
#include <unistd.h>
#include <fuse.h>


int
gn_truncate (const char *path, off_t size)
{
  int ret = 0;
  return ret;
}
