# WebAssembly

In this section, you will learn the basics of WebAssembly. More details about WebAssembly can be found in [Appendix B](../../appendix/webassembly_details.md)

## The origin story of WebAssembly

Like its name suggest, in a sense, it is related to the Web and Assembly. Web means that it, like many other forerunners, like asm.js, trying to improve JavaScript's performance in Browsers. And the Assembly means that the format of WebAssembly is not a human-readable format but a compact binary format that is more efficient for Browsers to use.

## The other benefits of WebAssembly

Other than aiming for performance improvement, the other WebAssembly could benefit us by reusing the code in your preferred programming language other than JavaScript. Many programmers from the C/C++ world are excited to utilize the existing code base of their apps or libraries and bring them onto the Web.

What makes it even better is that, like Javascript, shortly after the appearance of WebAssembly, it is not limited to the browser. It could also be used server-side. Many WebAssembly runtimes are out there, including our project WAMR.

## How does it work

### A browser example

The most straightforward place you could think of when it comes to the use of WebAssembly is in the browser.

Emscripten is a compiler toolchain for WebAssembly. It took the C/C++(or any other programming language LLVM frontend support) source program as input and translated it into the WebAssembly target program module.

Optionally, an HTML and a Javascript file are generated alongside a wasm file, so the plumbing JS code is ready for you to call your wasm module. And you could open HTML on your browser to see the result of your wasm program.

Here is the more detailed [emscripten official tutorial](https://emscripten.org/docs/getting_started/Tutorial.html) you could follow to write your hello world wasm program and run it on the browser.

### A server-side example

A hello world example using our WAMR can be found [here](../getting-started/README.md)
