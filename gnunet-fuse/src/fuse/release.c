/*
 * release.c - FUSE release function
 *
 *  Created on: Mar 14, 2012
 *      Author: mg
 *
 *
 *	Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.	 It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */

#include <fuse.h>
//#include <gnunet-fuse.h>

int
gn_release (const char *path, struct fuse_file_info *fi)
{
  return 0;
}
