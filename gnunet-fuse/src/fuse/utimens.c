/*
 * utimens.c - FUSE utimens function
 *
 *  Created on: Mar 14, 2012
 *      Author: mg
 *
 *
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * Introduced in version 2.6
 */

#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <fuse.h>



int
gn_utimens (const char *path, const struct timespec ts[2])
{
  int ret = 0;
  return ret;
}
