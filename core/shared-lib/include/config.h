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

#ifndef _CONFIG_H_

/* Memory allocator ems */
#define MEM_ALLOCATOR_EMS 0

/* Memory allocator tlsf */
#define MEM_ALLOCATOR_TLSF 1

/* Default memory allocator */
#define DEFAULT_MEM_ALLOCATOR MEM_ALLOCATOR_EMS

/* Beihai log system */
#define BEIHAI_ENABLE_LOG 1

/* Beihai debugger support */
#define BEIHAI_ENABLE_TOOL_AGENT 1

/* Beihai debug monitoring server, must define
 BEIHAI_ENABLE_TOOL_AGENT firstly */
#define BEIHAI_ENABLE_TOOL_AGENT_BDMS 1

/* enable no signature on sdv since verify doesn't work as lacking public key */
#ifdef CONFIG_SDV
#define BEIHAI_ENABLE_NO_SIGNATURE 1
#else
#define BEIHAI_ENABLE_NO_SIGNATURE 0
#endif

/* WASM VM log system */
#define WASM_ENABLE_LOG 1

/* WASM Interpreter labels-as-values feature */
#define WASM_ENABLE_LABELS_AS_VALUES 1

/* Heap and stack profiling */
#define BEIHAI_ENABLE_MEMORY_PROFILING 0

/* Max app number of all modules */
#define MAX_APP_INSTALLATIONS 3

/* Default timer number in one app */
#define DEFAULT_TIMERS_PER_APP 20

/* Max timer number in one app */
#define MAX_TIMERS_PER_APP 30

/* Max resource registration number in one app */
#define RESOURCE_REGISTRATION_NUM_MAX 16

/* Max length of resource/event url */
#define RESOUCE_EVENT_URL_LEN_MAX 256

/* Default length of queue */
#define DEFAULT_QUEUE_LENGTH 50

/* Default watchdog interval in ms */
#define DEFAULT_WATCHDOG_INTERVAL (3 * 60 * 1000)

/* Workflow heap size */
/*
#define WORKING_FLOW_HEAP_SIZE 0
*/

/* Default/min/max heap size of each app */
#define APP_HEAP_SIZE_DEFAULT (48 * 1024)
#define APP_HEAP_SIZE_MIN (2 * 1024)
#define APP_HEAP_SIZE_MAX (1024 * 1024)

/* Default/min/max stack size of each app thread */
#define APP_THREAD_STACK_SIZE_DEFAULT (20 * 1024)
#define APP_THREAD_STACK_SIZE_MIN (16 * 1024)
#define APP_THREAD_STACK_SIZE_MAX (256 * 1024)

#endif
