/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

package main

import (
    "gitlab.alipay-inc.com/TNT_Runtime/ant-runtime/bindings/go/wamr"
    //"io/ioutil"
    "fmt"
    //"os"
)

var wasmBytes = []byte {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x03, 0x60,
    0x01, 0x7F, 0x01, 0x7F, 0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F, 0x60, 0x01,
    0x7F, 0x00, 0x02, 0x31, 0x04, 0x03, 0x65, 0x6E, 0x76, 0x04, 0x70, 0x75,
    0x74, 0x73, 0x00, 0x00, 0x03, 0x65, 0x6E, 0x76, 0x06, 0x6D, 0x61, 0x6C,
    0x6C, 0x6F, 0x63, 0x00, 0x00, 0x03, 0x65, 0x6E, 0x76, 0x06, 0x70, 0x72,
    0x69, 0x6E, 0x74, 0x66, 0x00, 0x01, 0x03, 0x65, 0x6E, 0x76, 0x04, 0x66,
    0x72, 0x65, 0x65, 0x00, 0x02, 0x03, 0x02, 0x01, 0x01, 0x04, 0x05, 0x01,
    0x70, 0x01, 0x01, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x13, 0x03,
    0x7F, 0x01, 0x41, 0xC0, 0x28, 0x0B, 0x7F, 0x00, 0x41, 0xBA, 0x08, 0x0B,
    0x7F, 0x00, 0x41, 0xC0, 0x28, 0x0B, 0x07, 0x2C, 0x04, 0x06, 0x6D, 0x65,
    0x6D, 0x6F, 0x72, 0x79, 0x02, 0x00, 0x0A, 0x5F, 0x5F, 0x64, 0x61, 0x74,
    0x61, 0x5F, 0x65, 0x6E, 0x64, 0x03, 0x01, 0x0B, 0x5F, 0x5F, 0x68, 0x65,
    0x61, 0x70, 0x5F, 0x62, 0x61, 0x73, 0x65, 0x03, 0x02, 0x04, 0x6D, 0x61,
    0x69, 0x6E, 0x00, 0x04, 0x0A, 0xB2, 0x01, 0x01, 0xAF, 0x01, 0x01, 0x03,
    0x7F, 0x23, 0x80, 0x80, 0x80, 0x80, 0x00, 0x41, 0x20, 0x6B, 0x22, 0x02,
    0x24, 0x80, 0x80, 0x80, 0x80, 0x00, 0x41, 0x9B, 0x88, 0x80, 0x80, 0x00,
    0x10, 0x80, 0x80, 0x80, 0x80, 0x00, 0x1A, 0x02, 0x40, 0x02, 0x40, 0x41,
    0x80, 0x08, 0x10, 0x81, 0x80, 0x80, 0x80, 0x00, 0x22, 0x03, 0x0D, 0x00,
    0x41, 0xA8, 0x88, 0x80, 0x80, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x00,
    0x1A, 0x41, 0x7F, 0x21, 0x04, 0x0C, 0x01, 0x0B, 0x20, 0x02, 0x20, 0x03,
    0x36, 0x02, 0x10, 0x41, 0x80, 0x88, 0x80, 0x80, 0x00, 0x20, 0x02, 0x41,
    0x10, 0x6A, 0x10, 0x82, 0x80, 0x80, 0x80, 0x00, 0x1A, 0x41, 0x00, 0x21,
    0x04, 0x20, 0x03, 0x41, 0x04, 0x6A, 0x41, 0x00, 0x2F, 0x00, 0x91, 0x88,
    0x80, 0x80, 0x00, 0x3B, 0x00, 0x00, 0x20, 0x03, 0x41, 0x00, 0x28, 0x00,
    0x8D, 0x88, 0x80, 0x80, 0x00, 0x36, 0x00, 0x00, 0x20, 0x02, 0x20, 0x03,
    0x36, 0x02, 0x00, 0x41, 0x93, 0x88, 0x80, 0x80, 0x00, 0x20, 0x02, 0x10,
    0x82, 0x80, 0x80, 0x80, 0x00, 0x1A, 0x20, 0x03, 0x10, 0x83, 0x80, 0x80,
    0x80, 0x00, 0x0B, 0x20, 0x02, 0x41, 0x20, 0x6A, 0x24, 0x80, 0x80, 0x80,
    0x80, 0x00, 0x20, 0x04, 0x0B, 0x0B, 0x41, 0x01, 0x00, 0x41, 0x80, 0x08,
    0x0B, 0x3A, 0x62, 0x75, 0x66, 0x20, 0x70, 0x74, 0x72, 0x3A, 0x20, 0x25,
    0x70, 0x0A, 0x00, 0x31, 0x32, 0x33, 0x34, 0x0A, 0x00, 0x62, 0x75, 0x66,
    0x3A, 0x20, 0x25, 0x73, 0x00, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77,
    0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00, 0x6D, 0x61, 0x6C, 0x6C, 0x6F, 0x63,
    0x20, 0x62, 0x75, 0x66, 0x20, 0x66, 0x61, 0x69, 0x6C, 0x65, 0x64, 0x00 }

func main() {
    err := wamr.Runtime().FullInit(false, nil, 1)
    if err != nil {
        return
    }

    wamr.Runtime().SetLogLevel(wamr.LOG_LEVEL_FATAL)

    module, err1 := wamr.NewModule(wasmBytes)
    if err1 != nil {
        fmt.Println(err1)
        wamr.Runtime().Destroy()
        return
    }

    instance, err2 := wamr.NewInstance(module, 16384, 16384)
    if err2 != nil {
        fmt.Println(err2)
        module.Destroy()
        wamr.Runtime().Destroy()
        return
    }

    argv := make([]uint32, 2)
    err3 := instance.CallFunc("main", 2, argv)
    if err3 != nil {
        fmt.Println(err3)
        module.Destroy()
        wamr.Runtime().Destroy()
        return
    }

    instance.Destroy()
    module.Destroy()
    wamr.Runtime().Destroy()
    return
}
