# WebAssembly

In this section, you will learn the basics of WebAssembly. More details about WebAssembly can be found in [Appendix A](../../appendix/webassembly-details.md)

## The origin story of WebAssembly

Like its name suggest, in a sense, it related to Web and Assembly. Web means that it like many other forerunner, like asm.js, trying to improve JavaScript's performance in Browser. And the Assembly means that the format of WebAssembly is not a human readable format but a compact binary format that is more efficient for browser to use.

## The other benefits of WebAssembly

Other than aiming for performance improvement, the other WebAssembly could benefit us is that reusing the code in your preferred programming language other than JavaScript. Many programmers from C/C++ world are excited to utilize existing code base of their apps or libraries and bring them onto the Web.

What make it even better is that like Javascript, shortly after the appearance of WebAssembly, it is not limited on browser. It could also be used server-side, many WebAssembly runtime are out there, including our project WAMR.

## How does it work

### A browser example

The most straightforward place you could think of when it comes to the use of WebAssembly is in browser.

Emscripten is a compiler toolchain to WebAssembly. It took the C/C++(or any other programming language LLVM frontend support) source program as input and translate it into WebAssembly target program module.

Optionally, a HTML and a Javascript file are generated along side with wasm file, so the pluming JS code are ready for you to use to call your wasm module. And you could open html on your browser to see the result of your wasm program.

Here is the a more detailed [emscripten official tutorial](https://emscripten.org/docs/getting_started/Tutorial.html) you could follow to write your hello world wasm program and run it on browser.

### A server side example

A hello world example using our WAMR can be fine [here](../getting_started/README.md)

<!-- ## Structure of its module -->
