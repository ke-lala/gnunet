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
 * Java Binding for libextractor.  Each Extractor instance
 * represents a set of meta data extraction plugins.
 *
 * @see Xtract
 * @see MetaData
 * @author Christian Grothoff
 */ 
public final class Extractor {
	

    private static final boolean warn_;

    /**
     * LE version.  0 if LE was compiled without JNI/Java support, in which
     * case we better not call any native methods...
     */
    private static final int version_;

    static {	
	// first, initialize warn_
	boolean warn = false;
	try {
	    if (System.getProperty("libextractor.warn") != null)
		warn = true;
	} catch (SecurityException se) {
	    // ignore
	} finally {
	    warn_ = true; // warn;
	}

	// next, load library and determine version_
	int ver = 0;
	try {
	    System.loadLibrary("extractor_java");
	} catch (UnsatisfiedLinkError ule) {
	    ver = -1;
	    warn("Did not find libextractor_java library: " + ule);
	}
	if (ver == 0) {
	    try {
		ver = getVersionInternal();
	    } catch (UnsatisfiedLinkError ule) {
		// warn: libextractor compiled without Java support
		warn("libextractor library compiled without Java support: " + ule);
	    }
	}
	version_ = ver;
    }    


    private static void warn(String warning) {
	if (warn_)
	    System.err.println("WARNING: " + warning);
    }


    /**
     * @return -1 if LE library was not found, 0 if LE library
     *  was found but compiled without JNI support, otherwise
     *  the LE version number
     */
    public static int getVersion() {
	return version_;
    }


    /**
     * Get the 'default' extractor, that is an extractor that loads
     * the default set of extractor plugins.
     */
    public static Extractor getDefault() {
	if (version_ > 0)
	    return new Extractor(loadDefaultInternal());
	return new Extractor(0);
    }


    /**
     * Get the 'empty' extractor, that is an extractor that does not
     * have any plugins loaded.  This is useful to manually construct
     * an Extractor from scratch.
     */
    public static Extractor getEmpty() {
	return new Extractor(0L);
    }


    /**
     * Handle to the list of plugins (a C pointer, long to support
     * 64-bit architectures!).
     */
    private long pluginHandle_;


    /**
     * Creates an extractor.
     *
     * @param pluginHandle the internal handle (C pointer!) refering
     *   to the list of plugins.  0 means no plugins.
     */
    private Extractor(long pluginHandle) {
	pluginHandle_ = pluginHandle;
    }


    /**
     * Unloads all loaded plugins on "exit".
     */
    protected void finalize() {
	if (pluginHandle_ != 0)
	    unloadAllInternal(pluginHandle_);
    }


    /**
     * Remove a plugin from the list of plugins.
     *
     * @param pluginName name of the plugin to unload
     */
    public void unloadPlugin(String pluginName) {
	if (pluginHandle_ != 0) 
	    pluginHandle_ = unloadPluginInternal(pluginHandle_,
						 pluginName);	
    }


    /**
     * Add an additional plugin to the list of plugins
     * used.
     *
     * @param pluginName name of the plugin to load
     */
    public void loadPlugin(String pluginName) {
	if (version_ <= 0)
	    return; 
	pluginHandle_ = loadPluginInternal(pluginHandle_,
					   pluginName);
    }


    /**
     * Extract keywords (meta-data) from the given file.
     *
     * @param f the file to extract meta-data from
     * @return extracted meta data (ArrayList<MetaData>)
     */
    public ArrayList extract(File f) {
	return extract(f.getAbsolutePath());
    }


    /**
     * Extract keywords (meta-data) from the given file.
     *
     * @param file the name of the file
     * @return extracted meta data (ArrayList<MetaData>)
     */
    public ArrayList extract(String filename) {
	ArrayList ret = new ArrayList(0);
	if (pluginHandle_ == 0)
	    return ret; // fast way out
	extractInternal(pluginHandle_,
			filename,
			null,
			ret);
	return ret;
    }

    
    /**
     * Extract keywords (meta-data) from the given block
     * of data.
     *
     * @param data the file data
     * @return extracted meta data (ArrayList<MetaData>)
     */
    public ArrayList extract(byte[] data) {
	ArrayList ret = new ArrayList(0);
	if (pluginHandle_ == 0)
	    return ret; // fast way out
	extractInternal(pluginHandle_,
			null,
			data,
			ret);	
	return ret;
    }

    
    /* ********************* native calls ******************** */

    private static native long unloadPluginInternal(long handle,
						    String pluginName);
    
    private static native long loadPluginInternal(long handle,
						  String pluginName);

    private static native long loadDefaultInternal();

    private static native void unloadAllInternal(long handle);
    
    private static native void extractInternal(long handle,
					       String filename,
					       byte[] data,
					       ArrayList result);

    private static native int getVersionInternal();

    /**
     * Not private since we use this from "MetaData".
     */
    static native String getTypeAsStringInternal(int type);

    /**
     * Not private since we use this from "MetaData".
     */
    static native int getMaxTypeInternal();

} // end of Extractor
