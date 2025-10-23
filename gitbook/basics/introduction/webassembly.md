# WebAssembly

In this section, you will learn the basics of WebAssembly. More details about WebAssembly can be found in [Appendix B](../../appendix/webassembly_details.md)

## Overview

Like its name suggest, in a sense, it is related to the Web and Assembly. Web means that it, like many other forerunners, like asm.js, trying to improve JavaScript's performance in Browsers. And the Assembly means that the format of WebAssembly is not a human-readable format but a compact binary format that is more efficient for Browsers to use. To conclude, WebAssembly (Wasm) is a compact, binary instruction format tailored for a stack-based virtual machine. It serves as a portable compilation target for various programming languages, enabling smooth deployment across both client and server environments on the web. It aims to provide benefits such as:

- High Performance
  
  Wasm is built for fast execution and compact encoding, allowing programs to be efficiently transmitted and quickly loaded. By leveraging the common hardware capabilities across platforms, WebAssembly can run at near-native speeds.

- Secure Execution Environment
  
  Wasm operates within a memory-safe, sandboxed environment, which can be implemented even inside existing JavaScript engines. When integrated into web applications, it adheres to the same-origin policy and browser-based permission models, ensuring robust security.

- Open and Debuggable Format
  
  Wasm is designed with a textual representation that aids in debugging, testing, and optimization. This human-readable format allows developers to experiment, learn, and even hand-code Wasm modules. When viewed on the web, this text format makes Wasm modules easily accessible for inspection.

- A Core Part of the Open Web
  
  Built to uphold the versionless and backward-compatible nature of the web, WebAssembly seamlessly interacts with JavaScript and can access web APIs. Beyond web applications, Wasm is versatile and supports other non-web environments as well.

In [The State of WebAssembly 2023](https://www.cncf.io/reports/the-state-of-webassembly-2023/) from SlashData, Linux Foundation, and the Cloud Native Computing Foundation, some key insights into the current status and adoption of WebAssembly can be found. Including the top reason why people want to use WebAssembly:

- Faster loading times 23%
- Exploring new use cases and technologies 22%
- Sharing code between projects 20%
- Improved performance over JavaScript 20%
- Efficient execution of computationally intensive tasks 19%
- Binaries run anywhere 18%
- Sandboxed security 18%
- Language agnostic 18%

What makes it even better is that, like Javascript, shortly after the appearance of WebAssembly, it is not limited to the browser. It could also be used server-side. Many WebAssembly runtimes are out there, including our project WAMR.

## How does it work

### A browser example

The most straightforward place you could think of when it comes to the use of WebAssembly is in the browser.

Emscripten is a compiler toolchain for WebAssembly. It took the C/C++(or any other programming language LLVM frontend support) source program as input and translated it into the WebAssembly target program module.

Optionally, an HTML and a Javascript file are generated alongside a wasm file, so the plumbing JS code is ready for you to call your wasm module. And you could open HTML on your browser to see the result of your wasm program.

Here is the more detailed [emscripten official tutorial](https://emscripten.org/docs/getting_started/Tutorial.html) you could follow to write your hello world wasm program and run it on the browser.

### A server-side example

A hello world example using our WAMR can be found [here](../getting-started/README.md)
