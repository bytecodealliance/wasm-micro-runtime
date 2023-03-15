# Additional Sematic Check

this doc records additional sematic checking information of the ts2wasm. We add restrictions for some features of TypeScript as below

| item                            | description                                                  |
| ------------------------------- | ------------------------------------------------------------ |
| nominal class                   | the operation between two nomial classes(no inheritance relationship) |
| closure with default parameters | innter function or closure with default parameters           |
| explicitly any                  | the operation between any type and non-any type              |
| operate between different types | for example, 1 + 'str'(plus operation between `number` type and `string` type) |
| invoke any object               | treat any type as an object and access its properties        |
| array without explicit type     | declare `Array` without a typeargument, for exampe `new Array()` |

if those rules above are triggered, an error will be throwed, and you will see details in log.

## details

here shows some examples and other details about additional sematic checking.

the format of error message as below.

`[error type] in [function name where the error occurred], flag , message xxx`

For example

```shell
[inner function with default parameters]: in [tests/samples/call-expression-case6|callInternalReturnTest|callReturnTest], flag 6, message: 'inner function has default parameters'
```

this message shows that in function `tests/samples/call-expression-case6|callInternalReturnTest|callReturnTest`, there occurs a function with defalut parameters error, it's error flag is 6.

the meaning of error flags as below.

1.

binary operation between any type and non-any type without explicity type cast. For example,

```typescript
let a: any = 10;
let b = 11;
b = a; // error
// b = a as nummber; // correct
```

2.

binary operation between different types(not contain any type).For example,

```typescript
let a = '123'
let b = 1;
let c = a + b; // error
```

3.

binary operation, and one of them is any object. For example,

```typescript
let a: any = {x: 1, y: false};
let b = 1;
let c = b + a.x; // error
// let c = b + a.x as number // correct
```

4.

function return type and type of return expression are nominal classes. For example,

```typescript
class A {
}
class B {
}
function foo(): A {
    //xxx
    return new B(); // error
}

```

5.

function return type is non-any type, type of return expression is any(without type cast). For example,

```typescript
function foo(): number {
    const a: any = 10;
    return a; // error
    // return a as number // correct
}
```

6.

closure has default parameters. For example,

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

7.

argument type and parameter type are nominal class types. For example,

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

8.

argument type is any type without explicitly type casting. For example,

```typescript
const a: any = 10;
function foo(x: number) {
    // xxx
}
foo(a); // error
// foo(a as number) // correct
```

9.

`new Array` without a explicit type argument. For example,

```typescript
const a: number[] = new Array(); // error
// const a: number[] = new Array<number>(); // correct
```



