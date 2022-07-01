/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

int power(int n){
	int start=7;
	for(int i=0;i<n;i++){
		start=start*(start+1);
	}
	return start;
}