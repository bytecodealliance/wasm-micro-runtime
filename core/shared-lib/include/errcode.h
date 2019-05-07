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
 * @file   errcode.h
 * @date   Wed Feb 29 18:58:30 2012
 *
 * @brief  Host-visible error code definition
 */

#ifndef BEIHAI_ERRCODE_H
#define BEIHAI_ERRCODE_H

/**
 * Responses to all remote requests from host to Beihai runtime has a
 * return error code, which is used to indicate the processing result:
 * successful or any error occurs.  The following definitions include
 * all those error codes that may be returned to host.
 */
enum {
    BHE_SUCCESS = 0x000, /* Successful */

    /* General errors: 0x100 */
    BHE_OUT_OF_MEMORY = 0x101, /* Out of memory */
    BHE_BAD_PARAMETER = 0x102, /* Bad parameters to native */
    BHE_INSUFFICIENT_BUFFER = 0x103,
    BHE_MUTEX_INIT_FAIL = 0x104,
    BHE_COND_INIT_FAIL = 0x105, /* Cond init fail is not return to
     * host now, it may be used later.
     */
    BHE_WD_TIMEOUT = 0x106, /* Watchdog time out */

    /* Communication: 0x200 */
    BHE_MAILBOX_NOT_FOUND = 0x201, /* Mailbox not found */
    BHE_MSG_QUEUE_IS_FULL = 0x202, /* Message queue is full */
    BHE_MAILBOX_DENIED = 0x203, /* Mailbox is denied by firewall */

    /* Applet manager: 0x300 */
    BHE_LOAD_JEFF_FAIL = 0x303, /* JEFF file load fail, OOM or file
     * format error not distinct by
     * current JEFF loading
     * process (bool jeff_loader_load).
     */
    BHE_PACKAGE_NOT_FOUND = 0x304, /* Request operation on a package,
     * but it does not exist.
     */
    BHE_EXIST_LIVE_SESSION = 0x305, /* Uninstall package fail because of
     * live session exist.
     */
    BHE_VM_INSTANCE_INIT_FAIL = 0x306, /* VM instance init fail when create
     * session.
     */
    BHE_QUERY_PROP_NOT_SUPPORT = 0x307, /* Query applet property that Beihai
     * does not support.
     */
    BHE_INVALID_BPK_FILE = 0x308, /* Incorrect Beihai package format */

    BHE_VM_INSTNACE_NOT_FOUND = 0x312, /* VM instance not found */
    BHE_STARTING_JDWP_FAIL = 0x313, /* JDWP agent starting fail */
    BHE_GROUP_CHECK_FAIL = 0x314, /* Group access checking fail*/

    /* Applet instance: 0x400 */
    BHE_UNCAUGHT_EXCEPTION = 0x401, /* uncaught exception */
    BHE_APPLET_BAD_PARAMETER = 0x402, /* Bad parameters to applet */
    BHE_APPLET_SMALL_BUFFER = 0x403, /* Small response buffer */

    /*TODO: Should be removed these UI error code when integrate with ME 9 */
    /* UI: 0x500 */
    BHE_UI_EXCEPTION = 0x501,
    BHE_UI_ILLEGAL_USE = 0x502,
    BHE_UI_ILLEGAL_PARAMETER = 0x503,
    BHE_UI_NOT_INITIALIZED = 0x504,
    BHE_UI_NOT_SUPPORTED = 0x505,
    BHE_UI_OUT_OF_RESOURCES = 0x506
};

#endif
