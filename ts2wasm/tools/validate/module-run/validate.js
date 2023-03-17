/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const moduleName = arguments[0];

const value = typeConvert(arguments[2], arguments[3]);
const exportFunc = arguments[4];
const parameters = [];
for (let i = 5; i < arguments.length; i += 2) {
    parameters.push(typeConvert(arguments[i], arguments[i + 1]));
}
const buf = read(moduleName, 'binary');

// env.strcmp
const strcmp = {
    env: {
        strcmp(a, b) {
            return a == b;
        },
    },
};

WebAssembly.instantiate(buf, strcmp).then((wasmModule) => {
    const func = wasmModule.instance.exports[exportFunc];
    const res = func.call(func, ...parameters);
    console.log(value === res);
});

function typeConvert(type, arg) {
    switch (type) {
        case '0': {
            // boolean, or pass 1 (0 or 1)
            switch (arg) {
                case 'false':
                    return 0;
                case 'true':
                    return 1;
                default:
                    console.error(
                        `the input argument is not a boolean: ${arg}`,
                    );
                    break;
            }
            break;
        }
        case '1': // number
            return parseFloat(arg);
        case '2': // string, currently not support
            return arg;
        default:
            console.error(
                `the input argument is not a boolean, number or string: [${type}: ${arg}]`,
            );
    }
}
