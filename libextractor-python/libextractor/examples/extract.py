#!/usr/bin/env python
"""extract.py

     This file is part of libextractor.
     (C) 2002, 2003, 2004, 2005 Vidyut Samanta and Christian Grothoff
     (C) 2017 Nikita Gillmann <nikita@n0.is>

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

xtract = extractor.Extractor()


def print_k(xt, plugin, type, format, mime, data, datalen):
    mstr = cast(data, c_char_p)
    # FIXME: this ignores 'datalen', not that great...
    # (in general, depending on the mime type and format, only
    # the first 'datalen' bytes in 'data' should be used).
    if (format == extractor.EXTRACTOR_METAFORMAT_UTF8):
        print("%s - %s" % (xtract.keywordTypes()[type], mstr.value))
    return 0


for arg in sys.argv[1:]:
    print("Keywords from %s:" % arg)
    xtract.extract(print_k, None, arg)
