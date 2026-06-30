/*
     This file is part of libextractor.
     Copyright (C) 2021 Christian Grothoff

     libextractor is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     libextractor is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libextractor; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
     Boston, MA 02110-1301, USA.
*/
/**
 * @file plugins/test_elf.c
 * @brief testcase for elf plugin
 * @author Christian Grothoff
 */
#include "platform.h"
#include "test_lib.h"


/**
 * Main function for the ELF testcase.
 *
 * @param argc number of arguments (ignored)
 * @param argv arguments (ignored)
 * @return 0 on success
 */
int
main (int argc, char *argv[])
{
  struct SolutionData elf_sol[] = {
    {
      EXTRACTOR_METATYPE_MIMETYPE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "application/x-executable",
      strlen ("application/x-executable") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_TARGET_ARCHITECTURE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "i386",
      strlen ("i386") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_RESOURCE_TYPE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "Executable file",
      strlen ("Executable file") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_LIBRARY_DEPENDENCY,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "libc.so.6",
      strlen ("libc.so.6") + 1,
      0
    },
    { 0, 0, NULL, NULL, 0, -1 }
  };
  struct ProblemSet ps[] = {
    { "testdata/chello-elf",
      elf_sol },
    { NULL, NULL }
  };
  return ET_main ("elf", ps);
}


/* end of test_elf.c */
