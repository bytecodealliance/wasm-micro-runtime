/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file   bni.h
 * @date   Mon Jul  2 16:54:58 2012
 *
 * @brief  Beihai native interface.
 */

#ifndef BNI_H
#define BNI_H

#include "bh_types.h"

/* Primitive types */
typedef uint8 jboolean;
typedef int8 jbyte;
typedef uint16 jchar;
typedef int16 jshort;
typedef int32 jint;
typedef int64 jlong;
typedef float jfloat;
typedef double jdouble;
typedef jint jsize;

/* Predefined Java class types.  */
struct _jobject;
typedef struct _jobject *jobject;
struct _jclass;
typedef struct _jclass *jclass;
struct _jstring;
typedef struct _jstring *jstring;

/* Values of jboolean: */
#define BNI_FALSE  0
#define BNI_TRUE   1

/**
 * Return the length of the array object.
 *
 * @param array Java array object
 *
 * @return the length of the Java array
 */
#define bni_array_length(array) ((jsize)((uint32)(array)->__length >> 2))

/**
 * Return the address of the first element of array object.
 *
 * @param array Java array object
 *
 * @return the address of the first element of array object
 */
#define bni_array_elem(array) ((array)->__elem)

/**
 * Find the Java class with given class name.
 *
 * @param name Java class name
 *
 * @return class object of the Java class if found, NULL otherwise
 *
 * @throws OutOfMemoryError if VM runs out of memory.
 */
jclass
bni_find_class(const char *name);

/**
 * Throw an exception of given class with message.
 *
 * @param clazz class object of a subclass of java.lang.Throwable
 * @param msg message for the exception or NULL if no message
 *
 * @return 0 if succeeds, nonzero otherwise
 */
jint
bni_throw_new(jclass clazz, const char *msg);

/**
 * Throw a NullPointerException.
 *
 * @throws NullPointerException
 */
void
bni_throw_npe(void);

/**
 * Throw an ArrayIndexOutOfBoundsException
 *
 * @param index the index used to access the array
 *
 * @throws ArrayIndexOutOfBoundsException
 */
void
bni_throw_aioobe(int index);

/**
 * Determine whether an exception is being thrown.
 *
 * @return exception object if exception is thrown, NULL otherwise
 */
jobject
bni_exception_occurred(void);

/**
 * Print the current exception to error-reporting channel.
 */
void
bni_exception_describe(void);

/**
 * Clear the currently thrown exception.
 */
void
bni_exception_clear(void);

/**
 * Return the Unicode character number of a string.
 *
 * @param str Java string object
 *
 * @return the Unicode character number of the string
 */
jsize
bni_string_length(jstring str);

/**
 * Return the length in bytes of the modified UTF-8 representation of
 * a string.
 *
 * @param str Java string object
 * @param start start offset in the string
 * @param len number of Unicode characters
 *
 * @return the UTF-8 length of the string
 *
 * @throws StringIndexOutOfBoundsException on index overflow.
 */
jsize
bni_string_utf_length(jstring str, jsize start, jsize len);

/**
 * Copies len number of Unicode characters beginning at offset start
 * to the given buffer buf.
 *
 * @param str Java string object
 * @param start start offset in the string
 * @param len number of Unicode characters to copy
 * @param buf buffer for storing the result
 */
void
bni_string_region(jstring str, jsize start, jsize len, jchar *buf);

/**
 * Translates len number of Unicode characters beginning at offset
 * start into modified UTF-8 encoding and place the result in the
 * given buffer buf.
 *
 * @param str Java string object
 * @param start start offset in the string
 * @param len number of Unicode characters to copy
 * @param buf buffer for storing the result
 *
 * @throws StringIndexOutOfBoundsException on index overflow.
 */
void
bni_string_utf_region(jstring str, jsize start, jsize len, char *buf);

/**
 * Translate Unicode characters into modified UTF-8 encoding and return
 * the result.
 *
 * @param str Java string object
 *
 * @return the UTF-8 encoding string if succeeds, NULL otherwise
 */
char *
bni_string_get_utf_chars(jstring str);

/**
 * Get the given Java object's class index.
 *
 * @param obj Java object
 *
 * @return -1 if obj is an array, class index of the object otherwise
 */
jint
bni_object_class_index(jobject obj);

/**
 * Allocate memory from the current instance's private heap.
 *
 * @param size bytes to allocate
 *
 * @return pointer to the allocated memory
 *
 * @throws OutOfMemoryError if VM runs out of memory.
 */
void*
bni_malloc(unsigned size);

/**
 * Allocate memory from the current instance's private heap and clear
 * to zero.
 *
 * @param size bytes to allocate
 *
 * @return pointer to the allocated memory
 *
 * @throws OutOfMemoryError if VM runs out of memory.
 */
void*
bni_calloc(unsigned size);

/**
 * Free the memory allocated from the current instance's private heap.
 *
 * @param ptr pointer to the memory in current instance's private heap
 */
void
bni_free(void *ptr);

#endif
