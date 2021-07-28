#!/usr/bin/env bash

cp ../../../core/iwasm/include/*.h ../wamr/packaged/include/
cp ../../../product-mini/platforms/linux/build/libvmlib.a ../wamr/packaged/lib/linux-amd64/libwamr.a

