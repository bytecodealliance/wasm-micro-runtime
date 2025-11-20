Contributing to WAMR
=====================
As an open-source project, we welcome and encourage the community to submit patches directly to the project. In our collaborative open source environment, standards and methods for submitting changes help reduce the chaos that can result from an active development community.
We want to make contributing to this project as easy and transparent as possible, whether it's:
- Reporting a bug
- the current state of the code
- Submitting a fix
- Proposing new features

License
=======
WAMR uses the same license as LLVM: the `Apache 2.0 license` with the LLVM
exception. See the LICENSE file for details. This license allows you to freely
use, modify, distribute and sell your own products based on WAMR.
Any contributions you make will be under the same license.

Code changes
===================
We Use Github Flow, So All Code Changes Happen Through Pull Requests. Pull requests are the best way to propose changes to the codebase. We actively welcome your pull requests:

 - If you've added / modified code:
   - Please provide tests to the test suite to validate the operation of your code, or point to existing test cases which do the same. 
   - Ensure that your new tests pass. This way we ensure that your contribution continues to work as you expected as future contributions are made by other contributors.
   - Ensure all the existing tests in the test suite pass. This way we can verify that your contribution doesn’t accidentally impact other contributions.
   - If your contribution is minor and you feel it does not need an additional test case, i.e. updating comments, formatting, simple refactoring, etc. then provide an explanation in your PR comment, i.e. “this is a minor change *explain the change*, and as such [ “is covered by” *list existing test cases* | “is except from addition test contribution”].
- Avoid using macros for different platforms. Use separate folders for source files to collect together different host platform logic.
- Put macro definitions inside share_lib/include/config.h if you have to use macro.
- Make sure your code lints and compliant to our coding style.
- Extend the application library is highly welcome.

Coding Style
===============================
Please use [K&R](https://en.wikipedia.org/wiki/Indentation_style#K.26R) coding style, such as 4 spaces for indentation rather than tabs etc.
We suggest using VS Code like IDE or stable coding format tools, like clang-format, to make your code compliant to the customized format(in .clang-format).

Report bugs
===================
We use GitHub issues to track public bugs. Report a bug by [open a new issue](https://github.com/intel/wasm-micro-runtime/issues/new).

Code of Conduct
===============

WAMR is a [Bytecode Alliance](https://bytecodealliance.org/) project, and follows the Bytecode Alliance's [Code of Conduct](CODE_OF_CONDUCT.md) and [Organizational Code of Conduct](ORG_CODE_OF_CONDUCT.md).
