// Copyright (C) 2019 Intel Corporation.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//    Licensed to the Apache Software Foundation (ASF) under one or more
//    contributor license agreements.  See the NOTICE file distributed with
//    this work for additional information regarding copyright ownership.
//    The ASF licenses this file to You under the Apache License, Version 2.0
//    (the "License"); you may not use this file except in compliance with
//    the License.  You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//   Author: Ivan Volosyuk
//
    .text
    .align 2
.globl invokeNative
    .type   invokeNative, @function
invokeNative:

    push    %ebp
    movl    %esp, %ebp
    push    %ecx
    movl    8(%ebp), %eax           /* eax = argv */
    movl    12(%ebp), %ecx          /* ecx = argc */
    test    %ecx, %ecx
    je      restore_ecx             /* if ecx == 0, skip pushing arguments */
    leal    -4(%eax,%ecx,4), %eax   /* eax = eax + ecx * 4 - 4 */
    subl    %esp, %eax              /* eax = eax - esp */
1:
    push    0(%esp,%eax)
    loop    1b                      /* loop ecx counts */
restore_ecx:
    movl    -4(%ebp), %ecx          /* restore ecx */
    movl    16(%ebp), %eax          /* eax = func_ptr */
    call    *%eax
    leave
    ret

