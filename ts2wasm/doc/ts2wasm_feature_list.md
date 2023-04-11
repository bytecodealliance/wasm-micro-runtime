# ts2wasm feature list

## Type System
- Static layout, static access
    - Primitive
    - Class
    - Object literal
- Static layout, dynamic access
    - Interface
- dynamic layout, dynamic access
    - Any

## Function
- [x] function declaration
- [x] function expression
- [x] arrow function
- [x] closure
- [ ] default parameter
    - [x] top-level function
    - [ ] class method
    - [ ] closure
- [ ] optional parameter
- [x] rest parameter

## Class
- [x] class declaration
- [X] getter/setter
- [x] member field
- [x] member field initialization
- [x] member method
- [x] static method
- [x] static field
- [x] inherit
- [x] method override
- [x] implicit constructor
- [x] visibility control
- [ ] [index signatures](https://www.typescriptlang.org/docs/handbook/2/classes.html#index-signatures)
- [ ] [static blocks in class](https://www.typescriptlang.org/docs/handbook/2/classes.html#static-blocks-in-classes)
- [ ] [parameter properties](https://www.typescriptlang.org/docs/handbook/2/classes.html#parameter-properties)
- [ ] [class expressions](https://www.typescriptlang.org/docs/handbook/2/classes.html#class-expressions)
- [ ] implicit inherit (not support by design)

## Array
- [x] new array
- [x] array literal
- [x] access elements
- [ ] builtin method (push, slice...)

## Interface
- [x] implicit implements
- [x] access field
- [x] call method
- [x] assign interface to interface
- [x] assign class to interface
- [x] assign interface to class (require runtime check)
- [ ] [extending types](https://www.typescriptlang.org/docs/handbook/2/objects.html#extending-types)
- [ ] [intersection types](https://www.typescriptlang.org/docs/handbook/2/objects.html#intersection-types)

## Any
- [x] assign static value to any
- [x] assign any to static value (require runtime check)
- [x] operation (require runtime check)

## Type assertion
- [x] cast any to static (require runtime check)
- [ ] cast static to other static (not support by design)

## Module
- [x] export
    - [x] export to other source code
    - [x] export to host environment
- [x] import from other ts file
- [x] default import/export
- [x] import from host environment

## Decorator
Not supported

## Generic
Not supported

## Map
Not supported

## Tuple
Not supported

## Enums
Not supported

## Control flow
- [x] if/else
- [x] do
- [x] while
- [x] switch
- [x] break
- [ ] continue
- [ ] exception
- [ ] async/await

## Assignment
- [x] =
- [x] +=
- [x] -=
- [x] *=
- [x] /=
- [ ] %=
- [ ] <<=
- [ ] >>=
- [ ] >>>=
- [ ] &=
- [ ] ^=
- [ ] |=

## Operations
- logic
    - [x] ==
    - [x] !=
    - [x] >
    - [x] >=
    - [x] <
    - [x] <=
    - [ ] ===
    - [ ] !==
    - [x] &&
    - [x] ||
    - [x] !
- bitwise
    - [x] &
    - [x] |
    - [ ] ^
    - [ ] ~
    - [x] <<
    - [ ] >>
    - [ ] >>>
- arithmetical
    - [x] +
    - [x] -
    - [x] *
    - [x] /
    - [x] %

## Builtin objects
- [ ] Array
    - [x] isArray
    - [x] length
    - [ ] ...
- [ ] String
    - [x] concat
    - [x] slice
    - [x] length
    - [ ] ...
- [ ] console
    - [x] log
    - [ ] ...
- [ ] Math
    - [x] pow
    - [x] max
    - [x] min
    - [x] sqrt
    - [x] abs
    - [x] ceil
    - [x] floor
    - [x] trunc
    - [ ] ...
