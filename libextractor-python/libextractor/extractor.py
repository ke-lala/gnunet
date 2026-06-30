# -*- coding: utf-8 -*-
# Python bindings for GNU libextractor
#
# Copyright (C) 2006 Bader Ladjemi <bader@tele2.fr>
# Copyright (C) 2011 Christian Grothoff <christian@grothoff.org>
# Copyright (C) 2017, 2018 Nikita Gillmann <nikita@n0.is>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, write to the
# Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
# USA.

"""
Python bindings for GNU libextractor

libextractor is a simple library for keyword extraction.  libextractor
does not support all formats but supports a simple plugging mechanism
such that you can quickly add extractors for additional formats, even
without recompiling libextractor. libextractor typically ships with a
dozen helper-libraries that can be used to obtain keywords from common
file-types.

libextractor is a part of the GNU project (http://www.gnu.org/).
"""
from ctypes import *
from ctypes import CDLL
from ctypes.util import find_library
import os
import logging
import faulthandler

faulthandler.enable()

# Can be useful:
# "DYLD_LIBRARY_PATH" in os.environ

# fake cdll import
# loading shared object file
try:
    if os.uname()[0] == 'Linux':
        libextractor = CDLL('libextractor.so.3')
    elif os.uname()[0] == 'Linux':
        libextractor = cdll.LoadLibrary(find_library('libextractor.so.3'))
    else:
        libextractor = cdll.extractor
except:
    raise ImportError("Could not find shared 'libextractor' library.")


__all__ = ['Extractor']
__version__ = "1.7"
__licence__ = "GNU GPL"

"""
keyword's charset encoding
"""
KeywordType = c_int
MetaType = c_int

EXTRACT_CB = CFUNCTYPE(c_int, c_void_p, c_char_p, KeywordType, MetaType, c_char_p, c_void_p, c_size_t)

libextractor.EXTRACTOR_metatype_get_max.restype = KeywordType
libextractor.EXTRACTOR_metatype_to_description.restype = c_char_p
libextractor.EXTRACTOR_metatype_to_string.restype = c_char_p
libextractor.EXTRACTOR_plugin_add_defaults.restype = c_void_p
libextractor.EXTRACTOR_extract.argtypes = [c_void_p, c_char_p, c_void_p, c_size_t, EXTRACT_CB, c_void_p]


EXTRACTOR_METAFORMAT_UNKNOWN = 0
EXTRACTOR_METAFORMAT_UTF8 = 1
EXTRACTOR_METAFORMAT_BINARY = 2
EXTRACTOR_METAFORMAT_C_STRING = 3


class Extractor(object):
    """
    Main class for extracting meta-data with GNU libextractor.

    You may create multiple instances of Extractor to use
    different sets of library.  Initially each Extractor
    will start with the default set of libraries.

    Use the extract method to obtain keywords from a file.

    Use the add and remove libraries methods to change the list of
    libraries that should be used.
    """

    def __init__(self, defaults=True, libraries=None):
        """
        Initialize Extractor's instance

        @param libraries: list of strings that contains extractor's name (supported types)
        @param defaults: load default plugins
        """
        self.extractors = None
        if defaults:
            self.extractors = libextractor.EXTRACTOR_plugin_add_defaults(0)
        if libraries:
            self.extractors = libextractor.EXTRACTOR_plugin_add_config(self.extractors, libraries, 0)

    def extract(self, proc, proc_cls, filename=None, data=None, size=0):
        """
        Extract keywords from a file, or from its data.

        @param filename: filename string
        @param data: data contents
        @param size: data size
        @param proc: function to call on each value
        @param proc_cls: closure to proc

        If you give data, size has to be given as well.

        """
        if not filename and not (data and size):
            return None
        else:
            libextractor.EXTRACTOR_extract(self.extractors, filename, data, size, EXTRACT_CB(proc), proc_cls)

    def addLibrary(self, library):
        """
        Add given library to the extractor. Invoke with a string with the name
        of the library that should be added.  For example,

        'libextractor_filename'

        will prepend the extractor that just adds the filename as a
        keyword.

        No errors are reported if the library is not
        found.

        @param library: library's name
        """
        self.extractors = libextractor.EXTRACTOR_plugin_add(self.extractors, library, NULL, 0)

    def removeLibrary(self, library):
        """
        Remove a library.  Pass the name of the library that is to
        be removed.  Only one library can be removed at a time.
        For example,

        'libextractor_pdf'

        removes the PDF extractor (if added).
        ValueError will be thrown if no library match.

        @param library: library's name
        """

        self.extractors = libextractor.EXTRACTOR_plugin_remove(self.extractors, library)

    def addLibraries(self, libraries):
        """
        Add given libraries.
        Same as addLibary but libraries is a list of library's names.

        @param libraries: list of libraries names
        """

        self.extractors = libextractor.EXTRACTOR_plugin_add_config(self.extractors, libraries)

    def removeAllLibraries(self):
        """
        Remove all libraries.
        """

        libextractor.EXTRACTOR_plugin_remove_all(self.extractors)
        self.extractors = None

    def keywordTypes(self):
        """
        Returns the list of all keywords types.
        @return: list of all keywords types
        """
        i = 0
        keyword_types = []

        while True:
            keyword_type = libextractor.EXTRACTOR_metatype_to_string(i)
            if not keyword_type:
                break
            keyword_types.append(keyword_type)
            i += 1

        return tuple(keyword_types)

    def __del__(self):
        """
        >>> extractor = Extractor()
        >>> del extractor
        """
        if self.extractors:
            self.removeAllLibraries()


if __name__ == "__main__":
    try:
        import doctest
        doctest.testmod()
    except Exception as e:
        print(e)
