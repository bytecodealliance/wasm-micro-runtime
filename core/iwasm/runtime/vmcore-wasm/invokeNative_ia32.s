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

    .text
    .align 2
.globl invokeNative
    .type   invokeNative, @function
invokeNative:
    push    %ebp
    movl    %esp, %ebp
    movl    16(%ebp), %ecx          /* ecx = argc */
    movl    12(%ebp), %edx          /* edx = argv */
    test    %ecx, %ecx
    jz      skip_push_args          /* if ecx == 0, skip pushing arguments */
    leal    -4(%edx,%ecx,4), %edx   /* edx = edx + ecx * 4 - 4 */
    subl    %esp, %edx              /* edx = edx - esp */
1:
    push    0(%esp,%edx)
    loop    1b                      /* loop ecx counts */
skip_push_args:
    movl    8(%ebp), %edx           /* edx = func_ptr */
    call    *%edx
    leave
    ret

