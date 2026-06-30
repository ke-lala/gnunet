/*
     This file is part of libextractor.
     Copyright (C) 2002, 2003, 2004, 2007, 2010, 2012 Vidyut Samanta and Christian Grothoff

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
package org.gnu.libextractor;

import java.util.ArrayList;
import java.io.File;
import java.io.FileInputStream;

/**
 * An item of meta data extracted by GNU libextractor.
 *
 * @see Extractor
 * @author Christian Grothoff
 */ 
public final class MetaData {
   
    /**
     * Format is unknown.
     */
    public static final int METAFORMAT_UNKNOWN = 0;

    /**
     * 0-terminated, UTF-8 encoded string.  "data_len"
     * is strlen(data)+1.
     */
    public static final int METAFORMAT_UTF8 = 1;

    /**
     * Some kind of binary format, see given Mime type.
     */
    public static final int METAFORMAT_BINARY = 2;

    /**
     * 0-terminated string.  The specific encoding is unknown.
     * "data_len" is strlen(data)+1.
     */
    public static final int METAFORMAT_C_STRING = 3;


    /**
     * Cached list of Strings describing keyword types.
     */
    private final static String[] typeCache_;

    static {
	typeCache_ = new String[Extractor.getMaxTypeInternal()];
    }

    /**
     * LE type number for this meta data item.
     */
    public final int type;

    /**
     * LE format given for the meta data.
     */
    public final int format;

    /**
     * The meta data itself.
     */
    public final byte[] meta;

    /**
     * Mime-type of the meta data.
     */
    public final String meta_mime;


    /**
     * Constructor is only called from "native" code.
     *
     * @param t type of the meta data
     * @param f format of the meta data
     * @param m the actual meta data
     * @param mm mime type of the meta data
     */
    private MetaData (int t,
		      int f,
		      byte[] m,
		      String mm) {
	this.type = t;
	this.format = f;
	this.meta = m;
	this.meta_mime = mm;
    }


    public String toString() {
	return getTypeAsString() + " - " + getMetaDataAsString ();
    }


    /**
     * Return the meta data as a "String" (if the format
     * type permits this).
     *
     * @return null if the format of the meta data is unknown
     */
    public String getMetaDataAsString () {
	switch (format) {
	case METAFORMAT_C_STRING:
	    return new String (meta);
	case METAFORMAT_UTF8:
	    try {
		return new String (meta, "UTF-8");
	    } catch (java.io.UnsupportedEncodingException uee) {
		// Java should ALWAYS support UTF-8
		throw new Error (uee);
	    }	
	case METAFORMAT_BINARY:
	    return "(binary, " + meta.length + " bytes)";
	default:
	    return null;
	}
	     
    }


    /**
     * @return description of this meta data item as a string
     */
    public String getTypeAsString() {
	if (typeCache_[type] == null)
	    typeCache_[type]
		= Extractor.getTypeAsStringInternal(type);
	return typeCache_[type];
    }


} // end of MetaData
