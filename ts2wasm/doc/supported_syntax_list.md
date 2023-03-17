# ts2wasm syntax support list

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

## Function
- [x] function declaration
- [x] function expression
- [x] arrow function
- [x] closure
- [ ] default parameter
- [ ] rest parameter

## Class
- [x] class declaration
- [X] getter/setter
- [x] member field
- [x] member field initialization
- [x] member method
- [x] static method
- [ ] static field
- [x] inherit
- [x] implicit constructor
- [x] visibility control
- [ ] implicit inherit (not support by design)

## Array
- [x] new array
- [x] access elements
- [ ] builtin method (push, slice...)

## Interface
- [x] access field
- [ ] call method
- [x] assign interface to interface
- [x] assign class to interface
- [x] assign interface to class (require runtime check)

## Any
- [x] assign static value to any
- [x] assign any to static value (require runtime check)
- [x] operation (require runtime check)

## Type assertion
- [x] cast any to static (require runtime check)
- [ ] cast static to other static (not support by design)

## Module
- [x] export
- [x] import from other ts file
- [x] default import/export
- [x] import from host environment

## Decorator
Not supported

## Generic
Not supported

## Builtin objects
- [ ] Array
- [ ] String
    - WIP
- [ ] console
- [ ] Math
    - WIP
