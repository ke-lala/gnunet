"""
extract.py

     This file is part of libextractor.
     (C) 2002, 2003, 2004, 2005 Vidyut Samanta and Christian Grothoff
     (C) 2017, 2018 Nikita Gillmann <nikita@n0.is>

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
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.

Little demo how to use the libextractor Python binding.

"""
from __future__ import print_function
from libextractor import extractor
import sys
from ctypes import *
import struct
import logging
import faulthandler

faulthandler.enable()

xtract = extractor.Extractor()

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

formatter = logging.Formatter('%(asctime)s %(levelname)s - %(message)s')

fh = logging.FileHandler('log.txt')
fh.setLevel(logging.DEBUG)
fh.setFormatter(formatter)
logger.addHandler(fh)

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
ch.setFormatter(formatter)
logger.addHandler(ch)

def print_k(xt, plugin, type, format, mime, data, datalen):
    mstr = cast(data, c_char_p)
    # FIXME: this ignores 'datalen', not that great...
    # (in general, depending on the mime type and format, only
    # the first 'datalen' bytes in 'data' should be used).
    if (format == extractor.EXTRACTOR_METAFORMAT_UTF8):
        print("%s - %s" % (xtract.keywordTypes()[type], mstr.value))
        # DEBUGGING OUTPUT + LOG:
        logger.debug("%s - %s" % (xtract.keywordTypes()[type], mstr.value))
    return 0

def main():
    try:
        # stuff
        for arg in sys.argv[1:]:
            logger.debug("Keywords from %s:" % arg)
            xtract.extract(print_k, None, arg)
    except Exception as e:
        logger.debug(e)

if __name__ == "__main__":
    main()
