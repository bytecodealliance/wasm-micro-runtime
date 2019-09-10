Contributing to WAMR
=====================
As an open-source project, we welcome and encourage the community to submit patches directly to the project. In our collaborative open source environment, standards and methods for submitting changes help reduce the chaos that can result from an active development community.
We want to make contributing to this project as easy and transparent as possible, whether it's:
- Reporting a bug
- the current state of the code
- Submitting a fix
- Proposing new features

License
=====================
WAMR uses the permissive open source `Apache 2.0 license`_ that allows you to freely use, modify, distribute and sell your own products that include Apache 2.0 licensed software. 
Any contributions you make will be under the same license. Feel free to contact the maintainers if that's a concern.

Code changes
===================
We Use Github Flow, So All Code Changes Happen Through Pull Requests. Pull requests are the best way to propose changes to the codebase. We actively welcome your pull requests:

- If you've added code that should be tested, add tests. Ensure the test suite passes.
- Avoid use macros for different platforms. Use seperate folder of source files to host diffeent platform logic.
- Put macro definitions inside share_lib/include/config.h if you have to use macro.
- Make sure your code lints and compliant to our coding style.
- Extend the application library is highly welcome.

Coding Style
===============================
Please use [K&R](https://en.wikipedia.org/wiki/Indentation_style#K.26R) coding style, such as 4 spaces for indentation rather than tabs etc.
We suggest use Eclipse like IDE or stable coding format tools to make your code compliant to K&R format.

Report bugs
===================
We use GitHub issues to track public bugs. Report a bug by [open a new issue](https://github.com/intel/wasm-micro-runtime/issues/new).

Code of Conduct
===============

This project is governed by the [Contributor Covenant](CODE_OF_CONDUCT.md).
All contributors and participants agree to abide by its terms.
