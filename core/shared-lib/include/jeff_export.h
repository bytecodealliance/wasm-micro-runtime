/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file   jeff-export.h
 * @date   Wed Aug  3 18:17:30 2011
 *
 * @brief Exported interface for operating or executing JEFF files.
 * All interface names start with "jeff_", which is the namespace name
 * of this module.
 */

#ifndef JEFF_EXPORT_H
#define JEFF_EXPORT_H

#include "bni.h"
#include "bh_types.h"

/********************************************************************
 *                  Exported internal types
 ********************************************************************/

/**
 * JEFF file handle type
 */
struct JeffFileHeaderLinked;
typedef struct JeffFileHeaderLinked *jeff_file_t;

/**
 * JEFF class type
 */
struct JeffClassHeaderLinked;
typedef struct JeffClassHeaderLinked *jeff_class_t;

/**
 * VM instance handle type
 */
struct JeffInstanceLocalRoot;
typedef struct JeffInstanceLocalRoot *jeff_instance_t;

/**
 * Record of one native method's definition.
 */
struct JeffNativeMethodDef {
    /* Mangled name of the native method.  NULL for initialization
     functions.  */
    const char *mangled_name;

    /* Points to the native C function.  */
    void (*func_ptr)(uint32 *);

    /* Return size type of the native function.  */
    uint32 return_size_type;
};

/********************************************************************
 *    Interface for operating global environment of the JEFF VM
 ********************************************************************/

/**
 * Load the core library from the given file buffer and initialize the
 * runtime environment (global objects etc.) of the VM.  The thread
 * calls this function becomes the supervisor thread, which belongs to
 * a unique supervisor instance.  Currently, if this init failed,
 * partially initialized states of the VM runtime environment won't be
 * cleaned up, so the VM must be shutdown and restarted.  method_defs
 * points to an array of native method definition records.
 * Initialization functions must be in the front of the array and
 * following native method definitions must be sorted by their mangled
 * names.
 *
 * @param file the JEFF file of the core library
 * @param file_size the size of the JEFF file of the core library
 * @param method_defs native method definition records
 * @param method_defs_num number of native method records
 * @param heap the heap for the current (supervisor) instance
 *
 * @return true if succeeds, otherwise the error cannot be recovered
 */
bool
jeff_runtime_init(jeff_file_t file, unsigned file_size,
        struct JeffNativeMethodDef *method_defs, unsigned method_defs_num,
        void *heap);

/**
 * Load a JEFF file into the VM from the given file buffer.  It can be
 * called from any VM thread.
 *
 * @param file the JEFF file to be loaded
 * @param size the size of the JEFF file
 * @param is_library whether the JEFF file is a library
 * @param allow_to_load a function that returns true if classes in the
 * given package is allowed to be loaded.  The NULL function pointer
 * allows all packages.
 * @param allow_to_link a function that returns true if classes in the
 * given package is allowed to be linked to.  The NULL function
 * pointer allows all packages.
 *
 * @return true if succeeds, otherwise detailed error information is
 * passed to vmci_diagnostic_print.  The caller can catch it by
 * implementing that function.
 */
bool
jeff_runtime_load(jeff_file_t file, unsigned size, bool is_library,
        bool (*allow_to_load)(const uint8 *pname, unsigned len),
        bool (*allow_to_link)(const uint8 *pname, unsigned plen,
                const uint8 *cname, unsigned clen));

/**
 * Unload a JEFF file from the VM.  All resources related to the JEFF
 * file except the JEFF file itself are released.  It can be called
 * from any VM thread.
 *
 * @param file the JEFF file to be unloaded
 *
 * @return true if succeeds, otherwise detailed error information is
 * passed to vmci_diagnostic_print.  The caller can catch it by
 * implementing that function.
 */
bool
jeff_runtime_unload(jeff_file_t file);

/**
 * Return the JEFF file with the given file uid.
 *
 * @param fuid the unique id of a loaded JEFF file
 *
 * @return the JEFF file is exists, otherwise NULL
 */
jeff_file_t
jeff_runtime_fuid_to_file(unsigned fuid);

/**
 * Return the file uid of the given JEFF file.
 *
 * @param file a loaded JEFF file
 *
 * @return the unique id of the given JEFF file
 */
unsigned
jeff_runtime_file_to_fuid(jeff_file_t file);

/**
 * Create a supervisor thread belonging to the supervisor instance.
 * Threads that may interact with VM core must be either the main
 * thread of supervisor instance (which calls jeff_runtime_init) or
 * created by this function so that VM core required data structures
 * can be set up correctly.
 *
 * @param start_routine the start routine of the new thread
 * @param arg argument to the start routine
 *
 * @return true if succeeds, false otherwise
 */
bool
jeff_runtime_create_supervisor_thread(void* (*start_routine)(void *),
        void *arg);

/**
 * Create a supervisor thread belonging to the supervisor instance.
 * Threads that may interact with VM core must be either the main
 * thread of supervisor instance (which calls jeff_runtime_init) or
 * created by this function so that VM core required data structures
 * can be set up correctly.
 *
 * @param start_routine the start routine of the new thread
 * @param arg argument to the start routine
 * @param prio thread priority
 *
 * @return true if succeeds, false otherwise
 */
bool
jeff_runtime_create_supervisor_thread_with_prio(void* (*start_routine)(void *),
        void *arg, int prio);

/********************************************************************
 *      Interface for operating instance local environment
 ********************************************************************/

/**
 * Create a VM instance with the given JEFF file as its main file,
 * (i.e. the file containing the main class of the VM instance).  This
 * function can be called from any VM thread, but it must be isolated
 * from JEFF file's unloading operation so that the main file won't be
 * unloaded before it's locked by the new instance.  All instance
 * local memory except stacks of threads are allocated from the given
 * heap.  If succeeds, it increases reference count of the main_file
 * and returns the handle of the new VM instance.  The new instance's
 * main thread will run the start_routine with argument arg.  If the
 * cleanup_routine is not NULL, it will be called after start_routine
 * returns and just before the main thread exits.  It will also be
 * called after the instance is destroied.  It is guaranteed to be
 * called exactly once no matter how the instance terminates.
 *
 * @param main_file the main JEFF file of the new instance
 * @param heap the private heap of the new instance
 * @param stack_depth the maximal nesting levels of Java methods of
 * the new instance.  It must be <= 16 * 1024.  Otherwise the instance
 * creation will fail.
 * @param start_routine start routine of the main thread.  Don't
 * destroy the heap or inform other thread to do this at the end of
 * this routine since after it returns, VM core will call destroy
 * functions on objects allocated in this heap (e.g. locks and
 * condition variables).  Do the destroying or informing of destroying
 * in the cleanup_routine.
 * @param arg the instance argument that will be passed to the start
 * routine.  It can be get or set by jeff_runtime_get_instance_arg and
 * jeff_runtime_set_instance arg from threads of the instance.  The
 * caller can use it to store instance local data.
 * @param cleanup_routine the optional cleanup routine for the
 * instance, which may be NULL.  It may be executed in the end of the
 * main thread of the created instance by this function if this
 * instance exits normally, or it may be executed in a thread of other
 * instance in case this instance is being killed by that instance.
 * In both cases, this routine regards it is executed in a thread of
 * this instance (the instance created by this function) because
 * jeff_runtime_get_instance_arg will always return the argument of
 * this instance.
 *
 * @return the VM instance handle if succeeds, NULL otherwise
 */
jeff_instance_t
jeff_runtime_create_instance(jeff_file_t main_file, void *heap,
        unsigned stack_depth, void* (*start_routine)(void *), void *arg,
        void (*cleanup_routine)(void));

/**
 * Destroy the given VM instance and decrease the reference count of
 * its main file and all explicitly used JEFF files.  It can be called
 * from any VM thread.  If there are alive threads of the instance,
 * they will be terminated mandatorily and then the cleanup routine is
 * called if it's not NULL.
 *
 * @param handle the handle of the instance to be destroyed
 */
void
jeff_runtime_destroy_instance(jeff_instance_t handle);

/**
 * Retrieve the current instance's argument.
 *
 * @return the current instance's argument
 */
void*
jeff_runtime_get_instance_arg(void);

/**
 * Set the current instance's argument.
 *
 * @return the new argument for the current instance
 */
void
jeff_runtime_set_instance_arg(void *arg);

/**
 * Retrieve the current instance's heap.
 *
 * @return the current instance's heap
 */
void*
jeff_runtime_get_instance_heap(void);

/**
 * Suspend all threads of the given VM instance.  This function can
 * only be called from thread that is not of the given VM instance.
 *
 * @param handle the handle of the instance to be suspended
 */
void
jeff_runtime_suspend_instance(jeff_instance_t handle);

/**
 * Resume all threads of the given VM instance.  This function can
 * only be called from thread that is not of the given VM instance.
 *
 * @param handle the handle of the instance to be resumed
 */
void
jeff_runtime_resume_instance(jeff_instance_t handle);

/**
 * Interrupt all threads of the given VM instance.  This function can
 * only be called from thread that is not of the given VM instance.
 *
 * @param handle the handle of the instance to be interrupted
 * @param by_force whether the interruption is by force
 */
void
jeff_runtime_interrupt_instance(jeff_instance_t handle, bool by_force);

/**
 * Wait for the given VM instance to terminate.
 *
 * @param ilr the VM instance to be waited for
 * @param mills wait millseconds to return
 */
void
jeff_runtime_wait_for_instance(jeff_instance_t ilr, int mills);

/********************************************************************
 *       Interface for operating thread local environment
 ********************************************************************/

/**
 * Return true if there is an uncaught exception (thrown during
 * running an application or applet command).
 *
 * @return true if there is an uncaught exception
 */
bool
jeff_runtime_check_uncaught_exception(void);

/**
 * Print qualified name of the uncaught exception (and stack trace if
 * enabled) by calling vmci_diagnostic_print.
 */
void
jeff_runtime_print_uncaught_exception(void);

/**
 * Clear the uncaught exception.
 */
void
jeff_runtime_reset_uncaught_exception(void);

/**
 * Change current thread to a safe state (VMWAIT).  After calling this
 * and before calling jeff_runtime_exit_safe_state, all operations
 * must be safe, i.e. no GC or system level resource operations are
 * allowed because in a safe state, the VM instance is assumed to be
 * able to perform GC, JDWP or termination at any time.  Usually, this
 * function is called just before the native code is going to wait for
 * something and the exiting safe state function is called just after
 * the waiting returns.
 */
void
jeff_runtime_enter_safe_state(void);

/**
 * Change current thread to an unsafe state (RUNNING) so that unsafe
 * operations can also be done.
 */
void
jeff_runtime_exit_safe_state(void);

/**
 * Set thread local error code for the current thread.
 *
 * @param code the error code to be set
 */
void
jeff_runtime_set_error(unsigned code);

/**
 * Get the last error code of current thread.
 *
 * @return the last error code of current thread
 */
unsigned
jeff_runtime_get_error(void);

/********************************************************************
 *                  Interface for GC support
 ********************************************************************/

/**
 * Traverse all objects of the given heap that are global or locate in
 * threads' frames and return them by calling vmci_gc_rootset_elem.
 * This function will suspend all threads except the current one of
 * the VM instance owning the given heap before traversing.  It
 * traverses either all or none of the rootset objects, and returns
 * true and false respectively.  If it returns false, the GC process
 * shouldn't proceed and is not necessary to unmark anything because
 * no objects are marked.  The function jeff_runtime_gc_finished must
 * be called if and only if this function returns true so as to resume
 * threads that are suspended during GC process.
 *
 * @param heap the heap for which rootset objects are looked up
 *
 * @return true if succeeds, false otherwise
 */
bool
jeff_runtime_traverse_gc_rootset(void *heap);

/**
 * Get the reference offset table of the given object.  If the
 * returned value R >= 0, *ret points to the reference offset table of
 * the object and R is the number of offsets in the table.  Otherwise,
 * if the returned value R < 0, all reference fields of the object
 * must be in a continuous region (usually the object is an array),
 * then *ret is the offset to the first field in the region and R is
 * the number of such fields in the region.
 *
 * @param obj pointer to the Java object
 * @param ret points to a pointer for storing the reference offset
 * table if return value >= 0, or for storing the offset to the first
 * object reference in the Java object if return value < 0
 *
 * @return number of offsets in the reference_offset table if >= 0, or
 * number of object references in the object if < 0
 */
int
jeff_object_get_reference_offsets(const jobject obj, uint16 **ret);

/**
 * Inform the containing VM instance that GC has finished and all
 * suspended threads can be resumed.  This function must be called if
 * and only if jeff_runtime_traverse_gc_rootset returns true.
 */
void
jeff_runtime_gc_finished(void);

/********************************************************************
 *              Interface for tooling support
 ********************************************************************/

/**
 * This function is used to suspend the main thread of VM instance so
 * that debugger can have chance to connect to the VM instance, set
 * breakpoints and do any other debug settings.  It must be called
 * from the main thread of VM instance at the point just after VM
 * instance initialization finishes and just before application code
 * is to be executed.
 */
void
jeff_tool_suspend_self(void);

/**
 * Start up tool agent thread for the given VM instance.  It can be
 * called from any VM thread.
 *
 * @param handle the VM instance for which tool agent is started up
 * @param queue queue of the tool agent
 * @return true if succeeds, false otherwise
 */
bool
jeff_tool_start_agent(jeff_instance_t handle, void *queue);

/********************************************************************
 *              Interface for toolkit support
 ********************************************************************/

/**
 * Return the JEFF class pointer of the given class name.
 *
 * @param class_name the qualified class name
 *
 * @return the JEFF class pointer
 */
jeff_class_t
jeff_tool_get_jeff_class(const char *class_name);

/**
 * Get the mangled class name of the given class.
 *
 * @param clz the JEFF class
 * @param buf buffer for returning the mangled name
 * @param buf_size size of the buffer
 *
 * @return actual size of the mangled class name including the
 * terminating null byte
 */
unsigned
jeff_tool_get_mangled_class_name(jeff_class_t clz, char *buf,
        unsigned buf_size);

/**
 * Get class index of given class in its containing JEFF file.
 *
 * @param clz the JEFF class
 *
 * @return class index in the containing JEFF file
 */
int
jeff_tool_get_class_index(jeff_class_t clz);

/**
 * Callback handler prototype for traversing fields of class.
 *
 * @param arg argument passed to the handler from caller
 * @param access_flag access flag of the method
 * @param name the field name
 * @param descriptor mangled field type descriptor
 * @param offset the offset of the field in the class
 * @param size size of the field
 */
typedef void
(*JeffToolFieldHandler)(void *arg, unsigned access_flag, const char *name,
        const char *descriptor, unsigned offset, unsigned size);

/**
 * Traverse all fields of the given class, including those inherited
 * from super classes.  The fields are traversed in the same order as
 * the field layout of the class.
 *
 * @param arg argument to be passed to the handler
 * @param clz the JEFF class
 * @param instance instance fields or static fielts
 * @param handler the callback handler for each field
 */
void
jeff_tool_foreach_field(void *arg, jeff_class_t clz, bool instance,
        JeffToolFieldHandler handler);

/**
 * Callback handler prototype for traversing methods of class.
 *
 * @param arg argument passed to the handler from caller
 * @param access_flag access flag of the method
 * @param name mangled name of the method
 * @param descriptor mangled method arguments descriptor
 * @param retune_type mangled descriptor of method's return type
 */
typedef void
(*JeffToolMethodHandler)(void *arg, unsigned access_flag, const char *name,
        const char *descriptor, const char *return_type);

/**
 * Traverse all methods of the given class.
 *
 * @param arg argument to be passed to the handler
 * @param clz the JEFF class
 * @param handler the callback handler for each method
 */
void
jeff_tool_foreach_method(void *arg, jeff_class_t clz,
        JeffToolMethodHandler handler);

/**
 * Callback handler prototype for traversing classes of main file.
 *
 * @param arg argument passed to the handler from caller
 * @param clz pointer to one class in the main file
 */
typedef void
(*JeffToolClassHandler)(void *arg, jeff_class_t clz);

/**
 * Traverse all classes of the main file.
 *
 * @param arg argument to be passed to the handler
 * @param handler the callback handler for each class
 */
void
jeff_tool_foreach_class(void *arg, JeffToolClassHandler handler);

/********************************************************************
 *              Interface for executing applications
 ********************************************************************/

/**
 * Initialize global environment for executing Java applications.
 *
 * @return true if succeeds, false otherwise
 */
bool
jeff_application_env_init(void);

/**
 * Find the unique class containing a public static "main
 * ([Ljava.lang.String;)V" method from the main JEFF file of the
 * current instance and execute that method.
 *
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the main method is called, false otherwise (e.g. an
 * exception occurs when preparing the arguments Java string array)
 */
bool
jeff_application_execute(int argc, char *argv[]);

/********************************************************************
 *              Interface for executing applets
 ********************************************************************/

/**
 * Initialize global environment for executing applets.
 *
 * @return true if succeeds, false otherwise
 */
bool
jeff_applet_env_init(void);

/**
 * Start to run from com.intel.runtime.core.RuntimeContext.main with a
 * default message queue size and a default service class object.  If
 * the main JEFF file of the current VM instance contains exactly one
 * class that is derived from com.intel.util.IntelApplet, then use it
 * as the default service class.
 *
 * @param queue_size the default main message queue size
 * @param default_service_class qualified class name of the default
 * service class (entry point class), which must be in the main JEFF
 * file.  If NULL, find the default main class with rules described
 * above.
 *
 * @return true if succeeds, false otherwise
 */
bool
jeff_applet_start(int queue_size, const char *default_service_class);

#endif
