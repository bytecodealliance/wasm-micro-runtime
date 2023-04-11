# Additional Sematic Check

This doc records additional sematic checking information of the ts2wasm. We add restrictions for some features of TypeScript as below

| item                            | description                                                  |
| ------------------------------- | ------------------------------------------------------------ |
| nominal class                   | the operation between two nomial classes(no inheritance relationship) |
| inner closure with default parameters | innter function or closure with default parameters           |
| operate between non-any type and any type                 | the operation between any type and non-any type              |
| operate between different types | for example, 1 + 'str'(plus operation between `number` type and `string` type) |
| invoke any object               | treat any type as an object and access its properties        |
| array without a specified element type     | declare `Array` without a typeargument, for exampe `new Array()` |

If those rules above are triggered, an error will be throwed, the details will be dump to the log file.

The check of `array without a specified element type` will not be process in `sematic_check.ts`, because we lost the AST information when traversing `Expression`, it will be handled when parsing `new Array` on AST.

## Details

Here are some examples and other details about additional sematic checking.

the format of error message as below.

`[error type] in [function name where the error occurred], flag , message xxx`

For example

``` shell
[inner closure with default parameters]: in [tests/samples/call-expression-case6|callInternalReturnTest|callReturnTest], flag 6, message: 'inner function has default parameters'
```

this message shows that in function `tests/samples/call-expression-case6|callInternalReturnTest|callReturnTest`, there occurs a function with defalut parameters error, its error flag is 6.

the meaning of error flags as below.

1. Operation between any and non-primitive object without explicit casting. For example:

    ``` typescript
    Foo f;
    let a: any = f;
    let b: Foo = {...};
    b = a; // error
    // b = a as Foo; // correct
    ```

2. binary operation between different types(not contain any type), it aims to handling implicitly type casting. For example:

    ``` typescript
    let a = '123'
    let b = 1;
    let c = a + b; // error
    ```

3. invoke any type(point to an object). For example,

    ```typescript
    Foo f;
    let a: any = f;
    let b = 1;
    let c = b + a.x; // error
    // let c = b + (a as F).x // correct
    ```

4. inner function has default parameters. For example,

    ```typescript
    function callInternalReturnTest(a: number, b = 2) {
        function callReturnTest(a = 10, b = 1, c = 99) { // error
            return a + b + c;
        }
        // function callReturnTest(a: number, b: number, c: number) { // correct
        //   return a + b + c;
        // }
    }
    ```

5. operation on nominal class types(unless the two types have inheritance relationship). For example,

    ```typescript
    class A {
    }
    class B {
    }
    function foo(a: A) {
        //xxx
    }
    const b = new B();
    foo(b); // error
    ```

6. `new Array` without a explicit type argument. For example,

    ```typescript
    const a: number[] = new Array(); // error
    // const a: number[] = new Array<number>(); // correct
    ```
