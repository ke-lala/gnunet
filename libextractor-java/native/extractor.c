/*
     This file is part of libextractor.
     (C) 2002, 2003, 2004, 2005, 2010, 2012 Vidyut Samanta and Christian Grothoff

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

#include "config.h"
#include <extractor.h>
#include <jni.h>

/* gcj's jni.h does not define JNIEXPORT/JNICALL (at least
 * not in my version).  Sun defines it to 'empty' on GNU/Linux,
 * so that should work */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif

#include "org_gnu_libextractor_Extractor.h"

#define HIGHEST_TYPE_NUMBER EXTRACTOR_metatype_get_max()

/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    loadDefaultInternal
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL 
Java_org_gnu_libextractor_Extractor_loadDefaultInternal(JNIEnv * env,
							   jclass c) {
  return (jlong) (long) EXTRACTOR_plugin_add_defaults (EXTRACTOR_OPTION_DEFAULT_POLICY);
}

/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    unloadInternal
 * Signature: (J)V
 */
JNIEXPORT void JNICALL 
Java_org_gnu_libextractor_Extractor_unloadAllInternal(JNIEnv * env,
						         jclass c,
						        jlong arg) {
  EXTRACTOR_plugin_remove_all((struct EXTRACTOR_PluginList*) (long) arg);
}

/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    getTypeAsStringInternal
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL 
Java_org_gnu_libextractor_Extractor_getTypeAsStringInternal(JNIEnv * env,
							       jclass c,
							       jint type) {
  const char * str;

  if ( (type < 0) || (type > HIGHEST_TYPE_NUMBER) )
    return NULL; /* error! */
  str = EXTRACTOR_metatype_to_string((enum EXTRACTOR_MetaType)type);
  if (str == NULL)
    return NULL;
  return (*env)->NewStringUTF(env, str);
}

/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    getVersionInternal
 * Signature: ()I
 */
JNIEXPORT jint JNICALL 
Java_org_gnu_libextractor_Extractor_getVersionInternal(JNIEnv * env,
							  jclass c) {
  return EXTRACTOR_VERSION;
}

/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    getMaxTypeInternal
 * Signature: ()I
 */
JNIEXPORT jint JNICALL 
Java_org_gnu_libextractor_Extractor_getMaxTypeInternal(JNIEnv * env,
							  jclass c) {
  return HIGHEST_TYPE_NUMBER;
}

/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    unloadPlugin
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL 
Java_org_gnu_libextractor_Extractor_unloadPluginInternal(JNIEnv * env,
							    jclass c,
							    jlong handle,
							    jstring name) {
  const char * lname;
  jboolean bo;
  jlong ret;

  bo = JNI_FALSE;
  lname = (const char*) (*env)->GetStringUTFChars(env, name, &bo);
  ret = (jlong) (long) EXTRACTOR_plugin_remove((struct EXTRACTOR_PluginList*) (long) handle,
					       lname);
  (*env)->ReleaseStringUTFChars(env, name, lname);
  return ret;
}

/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    loadPlugin
 * Signature: (JLjava/lang/String;Z)J
 */
JNIEXPORT jlong JNICALL 
Java_org_gnu_libextractor_Extractor_loadPluginInternal(JNIEnv * env,
							  jclass c,
							  jlong handle,
							  jstring name) {
  const char * lname;
  jboolean bo;
  jlong ret;

  bo = JNI_FALSE;
  lname = (const char*) (*env)->GetStringUTFChars(env, name, &bo);
  ret = (jlong) (long) EXTRACTOR_plugin_add((struct EXTRACTOR_PluginList*) (long) handle,
					    lname,
					    NULL,
					    EXTRACTOR_OPTION_DEFAULT_POLICY);
  (*env)->ReleaseStringUTFChars(env, name, lname);
  return ret;
}


/**
 * Closure of 'add_meta' function.
 */
struct AddMetaContext
{
  JNIEnv *env;
  jobject ret;
  jclass metadata;
  jmethodID meta_ctor;
  jmethodID alist_add;
};


/**
 * Function called by libextractor for each meta data item.
 */
static int
add_meta (void *cls,
	  const char *plugin_name,
	  enum EXTRACTOR_MetaType type,
	  enum EXTRACTOR_MetaFormat format,
	  const char *data_mime_type,
	  const char *data,
	  size_t data_len)
{
  struct AddMetaContext *ctx = cls;
  jbyteArray bdata;
  jstring mimestring;
  jobject metadata;
  
  mimestring = (*ctx->env)->NewStringUTF (ctx->env,
					  data_mime_type);
  bdata = (*ctx->env)->NewByteArray (ctx->env,
				     data_len);
  (*ctx->env)->SetByteArrayRegion (ctx->env,
				   bdata,
				   0,
				   data_len,
				   (jbyte*) data);
  metadata = (*ctx->env)->NewObject (ctx->env,
				     ctx->metadata,
				     ctx->meta_ctor,
				     type, format, bdata, 
				     mimestring);
  (*ctx->env)->CallBooleanMethod (ctx->env,
				  ctx->ret,
				  ctx->alist_add,
				  metadata);
  return 0;
}


/*
 * Class:     org_gnu_libextractor_Extractor
 * Method:    extractInternal
 * Signature: (JLjava/lang/String;[BLjava/util/ArrayList;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_libextractor_Extractor_extractInternal
  (JNIEnv *env, jclass c, jlong arg, jstring f, jbyteArray ba, jobject ret)
{
  const char * fname;
  void * data;
  jboolean bo;
  jsize asize;
  struct AddMetaContext am_ctx;
  jclass alist;

  am_ctx.env = env;
  am_ctx.ret = ret;
  am_ctx.metadata = (*env)->FindClass (env, "org/gnu/libextractor/MetaData");
  if (am_ctx.metadata == 0)
    return;
  am_ctx.meta_ctor = (*env)->GetMethodID (env,
					  am_ctx.metadata, 
					  "<init>",
					  "(II[BLjava/lang/String;)V");
  if (am_ctx.meta_ctor == 0)
    return;
  alist = (*env)->FindClass (env, "java/util/ArrayList");
  if (alist == 0)
    return;
  am_ctx.alist_add = (*env)->GetMethodID (env, 
					  alist, 
					  "add",
					  "(Ljava/lang/Object;)Z");
  if (am_ctx.alist_add == 0)
    return;
  bo = JNI_FALSE;
  if (f != 0)
    {
      fname = (const char*) (*env)->GetStringUTFChars(env, 
						      f, 
						      &bo);
    }
  else
    {
      fname = NULL;
    }
  if (ba != 0)
    {
      asize = (*env)->GetArrayLength(env, ba);
      data = (*env)->GetPrimitiveArrayCritical(env, 
					       ba,
					       &bo);
    }
  else
    {
      asize = 0;
      data = 0;
    }
  if ( (data == NULL) && (fname == NULL) )
    return;
  EXTRACTOR_extract((struct EXTRACTOR_PluginList*) (long) arg,		    
		    fname,
		    data,
		    (size_t) asize,
		    &add_meta,
		    &am_ctx);
  if (fname != NULL)
    (*env)->ReleaseStringUTFChars(env, 
				  f,
				  fname);
  if (data != NULL)
    (*env)->ReleasePrimitiveArrayCritical(env, 
					  f, 
					  data,
					  JNI_ABORT);
}

