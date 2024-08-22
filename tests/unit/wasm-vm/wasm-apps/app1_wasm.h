/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

unsigned char app1_wasm[] = {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1E, 0x06, 0x60,
    0x01, 0x7F, 0x01, 0x7F, 0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F, 0x60, 0x01,
    0x7F, 0x00, 0x60, 0x03, 0x7F, 0x7F, 0x7F, 0x01, 0x7F, 0x60, 0x00, 0x00,
    0x60, 0x00, 0x01, 0x7F, 0x02, 0x40, 0x05, 0x03, 0x65, 0x6E, 0x76, 0x06,
    0x6D, 0x61, 0x6C, 0x6C, 0x6F, 0x63, 0x00, 0x00, 0x03, 0x65, 0x6E, 0x76,
    0x06, 0x63, 0x61, 0x6C, 0x6C, 0x6F, 0x63, 0x00, 0x01, 0x03, 0x65, 0x6E,
    0x76, 0x04, 0x66, 0x72, 0x65, 0x65, 0x00, 0x02, 0x03, 0x65, 0x6E, 0x76,
    0x06, 0x6D, 0x65, 0x6D, 0x63, 0x70, 0x79, 0x00, 0x03, 0x03, 0x65, 0x6E,
    0x76, 0x06, 0x73, 0x74, 0x72, 0x64, 0x75, 0x70, 0x00, 0x00, 0x03, 0x09,
    0x08, 0x04, 0x01, 0x05, 0x00, 0x01, 0x02, 0x03, 0x00, 0x05, 0x03, 0x01,
    0x00, 0x01, 0x06, 0x23, 0x06, 0x7F, 0x00, 0x41, 0x80, 0x08, 0x0B, 0x7F,
    0x00, 0x41, 0x80, 0x08, 0x0B, 0x7F, 0x00, 0x41, 0x80, 0x08, 0x0B, 0x7F,
    0x00, 0x41, 0x80, 0x28, 0x0B, 0x7F, 0x00, 0x41, 0x00, 0x0B, 0x7F, 0x00,
    0x41, 0x01, 0x0B, 0x07, 0xD4, 0x01, 0x10, 0x06, 0x6D, 0x65, 0x6D, 0x6F,
    0x72, 0x79, 0x02, 0x00, 0x11, 0x5F, 0x5F, 0x77, 0x61, 0x73, 0x6D, 0x5F,
    0x63, 0x61, 0x6C, 0x6C, 0x5F, 0x63, 0x74, 0x6F, 0x72, 0x73, 0x00, 0x05,
    0x07, 0x6F, 0x6E, 0x5F, 0x69, 0x6E, 0x69, 0x74, 0x00, 0x05, 0x07, 0x6D,
    0x79, 0x5F, 0x73, 0x71, 0x72, 0x74, 0x00, 0x06, 0x0C, 0x6E, 0x75, 0x6C,
    0x6C, 0x5F, 0x70, 0x6F, 0x69, 0x6E, 0x74, 0x65, 0x72, 0x00, 0x07, 0x09,
    0x6D, 0x79, 0x5F, 0x6D, 0x61, 0x6C, 0x6C, 0x6F, 0x63, 0x00, 0x08, 0x09,
    0x6D, 0x79, 0x5F, 0x63, 0x61, 0x6C, 0x6C, 0x6F, 0x63, 0x00, 0x09, 0x07,
    0x6D, 0x79, 0x5F, 0x66, 0x72, 0x65, 0x65, 0x00, 0x0A, 0x09, 0x6D, 0x79,
    0x5F, 0x6D, 0x65, 0x6D, 0x63, 0x70, 0x79, 0x00, 0x0B, 0x09, 0x6D, 0x79,
    0x5F, 0x73, 0x74, 0x72, 0x64, 0x75, 0x70, 0x00, 0x0C, 0x0C, 0x5F, 0x5F,
    0x64, 0x73, 0x6F, 0x5F, 0x68, 0x61, 0x6E, 0x64, 0x6C, 0x65, 0x03, 0x00,
    0x0A, 0x5F, 0x5F, 0x64, 0x61, 0x74, 0x61, 0x5F, 0x65, 0x6E, 0x64, 0x03,
    0x01, 0x0D, 0x5F, 0x5F, 0x67, 0x6C, 0x6F, 0x62, 0x61, 0x6C, 0x5F, 0x62,
    0x61, 0x73, 0x65, 0x03, 0x02, 0x0B, 0x5F, 0x5F, 0x68, 0x65, 0x61, 0x70,
    0x5F, 0x62, 0x61, 0x73, 0x65, 0x03, 0x03, 0x0D, 0x5F, 0x5F, 0x6D, 0x65,
    0x6D, 0x6F, 0x72, 0x79, 0x5F, 0x62, 0x61, 0x73, 0x65, 0x03, 0x04, 0x0C,
    0x5F, 0x5F, 0x74, 0x61, 0x62, 0x6C, 0x65, 0x5F, 0x62, 0x61, 0x73, 0x65,
    0x03, 0x05, 0x0A, 0x41, 0x08, 0x03, 0x00, 0x01, 0x0B, 0x0D, 0x00, 0x20,
    0x01, 0x20, 0x01, 0x6C, 0x20, 0x00, 0x20, 0x00, 0x6C, 0x6A, 0x0B, 0x04,
    0x00, 0x41, 0x00, 0x0B, 0x06, 0x00, 0x20, 0x00, 0x10, 0x00, 0x0B, 0x08,
    0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x01, 0x0B, 0x06, 0x00, 0x20, 0x00,
    0x10, 0x02, 0x0B, 0x0A, 0x00, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0x10,
    0x03, 0x0B, 0x06, 0x00, 0x20, 0x00, 0x10, 0x04, 0x0B, 0x00, 0x76, 0x09,
    0x70, 0x72, 0x6F, 0x64, 0x75, 0x63, 0x65, 0x72, 0x73, 0x01, 0x0C, 0x70,
    0x72, 0x6F, 0x63, 0x65, 0x73, 0x73, 0x65, 0x64, 0x2D, 0x62, 0x79, 0x01,
    0x05, 0x63, 0x6C, 0x61, 0x6E, 0x67, 0x56, 0x31, 0x31, 0x2E, 0x30, 0x2E,
    0x30, 0x20, 0x28, 0x68, 0x74, 0x74, 0x70, 0x73, 0x3A, 0x2F, 0x2F, 0x67,
    0x69, 0x74, 0x68, 0x75, 0x62, 0x2E, 0x63, 0x6F, 0x6D, 0x2F, 0x6C, 0x6C,
    0x76, 0x6D, 0x2F, 0x6C, 0x6C, 0x76, 0x6D, 0x2D, 0x70, 0x72, 0x6F, 0x6A,
    0x65, 0x63, 0x74, 0x20, 0x31, 0x37, 0x36, 0x32, 0x34, 0x39, 0x62, 0x64,
    0x36, 0x37, 0x33, 0x32, 0x61, 0x38, 0x30, 0x34, 0x34, 0x64, 0x34, 0x35,
    0x37, 0x30, 0x39, 0x32, 0x65, 0x64, 0x39, 0x33, 0x32, 0x37, 0x36, 0x38,
    0x37, 0x32, 0x34, 0x61, 0x36, 0x66, 0x30, 0x36, 0x29
};
