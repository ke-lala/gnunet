/*
 * write.c - FUSE write function
 *
 *  Created on: Mar 14, 2012
 *      Author: mg
 *
 *	 Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fuse.h>


int
gn_write (const char *path, const char *buf, size_t size, off_t offset,
	  struct fuse_file_info *fi)
{
  return -ENOENT;
}
