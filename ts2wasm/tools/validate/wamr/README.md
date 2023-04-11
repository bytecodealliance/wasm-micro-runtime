# validate execution on WAMR

This folder contains code to compile all the sample code and execute on WAMR.

## Run

Please ensure you have built the `iwasm_gc` in default directory, see this [document](../../../runtime-library/README.md)

``` bash
npm start
```

The script will print the pass rate, and the failed cases will be recorded into the `test.log` file.

### Typical errors
- `Running [xxx] get invalid return code: xxx`
    - This error means the `iwasm_gc` returned with non-zero code
    - if code is null, it means the runtime process crashed
- `Running [xxx] get unexpected output`
    - This means the actual output of the wasm module is not expected text

### How it works?

1. Compile the ts source code to wasm module
2. Run the wasm module on WAMR with given arguments in `validation.json`
3. Compare the output with the expected result

> Note: The failed cases doesn't always mean there are bugs in the compiler or runtime, many known reasons may cause the cases to fail:
>
>    1. Some cases require non-primitive arguments (e.g. struct, any) but currently the runtime only receives `number` type arguments through cli
>    2. Some cases require some imported functions/globals, but the runtime didn't provide them
>    3. Some cases return complex values (e.g. struct, array, any), the runtime can't print detailed information, so the output is not accurate

## validation.json format

`validation.json` is a json file to record the cases to be tested, the format is:

``` json
[
    {
        "module": "any_binary_add",
        "entries": [
            {
                "name": "addAnyAny",
                "args": [],
                "result": "3:f64"
            },
            {
                "name": "addNumberAnyInBinaryExpr",
                "args": [],
                "result": "2:f64"
            },
            {
                "name": "addNumberAnyInMulExpr",
                "args": [],
                "result": "9:f64"
            }
        ]
    },
    {
        // ...
    }
]
```

- `module` is the name of the ts file to be tested
- `entries` is an array of functions to be tested
    - `name` is the name of the exported function
    - `args` is an array of arguments, currently only support number
    - `result` is the expected output text

## Auto generate validation.json

> This is for develop usage, don't run this script unless you know what will happen.

It's tedious to write the `validation.json` file manually, so we provide a script to generate it automatically.

``` bash
npm run gen_item
```

### How it works?

1. The script use ts2wasm frontend to parse the ts source code, and extract all export functions, and get the parameter count of the function
2. For each function, it will randomly generate some argument and call it with `ts-node`
3. The generated args, and the executed result will be recorded into the `validation.json` file

> Note: This script will overwrite the `validation.json` file, so please backup it before running this script.

> Note: Some functions may not be able to run on `ts-node`, so the script will remain the `result` empty. Also some text may not be very accurate, you need to fill/correct it manually.
