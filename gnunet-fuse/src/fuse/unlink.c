/*
 * unlink.c - FUSE unlink function
 *
 *  Created on: Mar 14, 2012
 *      Author: mg
 *
 * Remove a file
 *
 * */

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>


int
gn_unlink (const char *path)
{
  return 0;
}
