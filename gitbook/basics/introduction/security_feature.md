# The Security of WebAssembly and WAMR's implementation

WebAssembly is a cutting-edge programming language that helps Web applications such as PhotoShop Online run at native speed in the browser and offers a sandbox mechanism to protect the host environment from malicious attacks cross the world. Beyond the browser, the Wasm can be executed in standalone runtime such as WAMR safely without the need of additional security support from OS and HW.

## WebAssembly Security Overview

The security features of WebAssembly(More detailed Wasm language-level security features can be found on the official [WebAssembly website](https://webassembly.org/docs/security/))

WebAssembly (Wasm) is designed with two key security goals:

1. protecting users from malicious or faulty modules

2. providing developers with robust tools for building secure applications.

### User Protection

Each WebAssembly module executes within a sandboxed environment separated from the host runtime using fault isolation techniques. This implies:

- Applications execute independently, and can't escape the sandbox without going through appropriate APIs.
- Applications generally execute deterministically with limited exceptions.

Modules must also comply with the security policies of the host environment, such as the same-origin policy in browsers or POSIX on other platforms.

### Developer Safety Tools

WebAssembly's design emphasizes security by removing unsafe execution features while maintaining compatibility with C/C++ programs.

Key safety features include:

- **Control-flow Integrity (CFI):**
  - Modules must declare all accessible functions and their types at load time, enforcing structured control flow.
  - Immutable, non-observable compiled code prevents control-flow hijacking attacks.
  
- **Function Calls:**
  - Calls must reference a valid function index.
  - Indirect function calls are checked at runtime for type signature compatibility.
  - A protected call stack prevents buffer overflows, ensuring safe returns.
  - Branches are restricted to valid destinations within the current function.

- **Variable Handling:**
  - Local variables (fixed scope) are initialized to zero and stored in the protected call stack.
  - Global variables are stored in the global index space and can be imported from external modules.
  - Variables with unclear static scope (e.g., structs or address-of operator) are stored in isolated linear memory, which has bounds checking and zero-initialization by default.

- **Traps:**
  - Used to terminate execution and signal errors (e.g., invalid index, type mismatch, stack overflow, out-of-bounds memory access, or illegal arithmetic).
  - In a browser, traps trigger a JavaScript exception. Future updates may support custom module-defined trap handlers.

Future improvements may introduce multiple memory sections and finer memory controls (e.g., shared memory, page protection).

### Memory Safety

WebAssembly improves memory safety by eliminating common bugs found in traditional C/C++ programs:

- **Buffer Overflows**: Local and global variables are fixed-size and stored by index, preventing buffer overflows from affecting adjacent memory. Linear memory regions, though, can overwrite objects, but bounds checking and control-flow integrity (CFI) prevent direct code injection, so mitigation like DEP or SSP is unnecessary.
  
- **Pointer Safety**: Unsafe pointer usage, like dereferencing unallocated memory or accessing freed memory, is minimized. WebAssembly removes pointer semantics for function calls and variables with fixed scope, and invalid index references result in load-time validation errors or runtime traps. Linear memory is bounds-checked at the region level and zero-initialized by default.

- **Control Flow Protection**: Although WebAssembly prevents direct code injection, code reuse attacks targeting indirect calls are still possible. However, conventional ROP attacks are infeasible due to CFI, which enforces valid call targets declared at load time.

- **Potential Vulnerabilities**: Race conditions (e.g., TOCTOU) and side-channel attacks (e.g., timing attacks) are possible, as WebAssembly offers no scheduling guarantees. Future enhancements may introduce memory randomization, code diversification, and bounded pointers to strengthen protection.

### Control-Flow Integrity (CFI)

Wasm ensures CFI for both direct and indirect function calls as well as function returns. It uses explicit function section indexes and runtime type checks to verify safe transitions. However, as mentioned above, while these mechanisms prevent most code injection, indirect call exploits using code reuse techniques are still possible.

Developers can enhance security by enabling fine-grained CFI through Clang/LLVM's WebAssembly support, which adds richer type-level checks and mitigates indirect call attacks, albeit with minor performance trade-offs.

## WAMR Security Features

The WebAssembly Micro Runtime (WAMR) is designed to provide an efficient, secure, and lightweight runtime for WebAssembly on standalone devices. It offers a full coverage of the WebAssembly specification and added additional security enhancements to ensure safe execution of Wasm modules.

### Wasm Language Security Features

WAMR enforces WebAssembly language-level security features rigorously, ensuring that each Wasm module undergoes comprehensive validation at the loading phase and that execution conforms to the WebAssembly specification during runtime.

#### Module Validation

Before execution, WAMR validates the Wasm module to ensure it adheres to the WebAssembly specification. This involves several key checks:

- **Format Validation**: Ensuring the binary is well-formed and compliant with the Wasm format. This checks the structure, ensuring correct definitions for functions, memory segments, tables, and types.
  
- **Type Checking**: All function signatures, local variables, and global variables are verified against their declared types. This ensures type safety across calls and memory operations.

- **Control Flow Integrity**: Verifies the function call graph to ensure that all function indices and signatures are valid and that function calls do not violate control-flow safety rules.

- **Operand Stack Integrity**: WAMR ensures that operand stack overflows and underflows are checked during validation. For each function, the number of values pushed and popped from the operand stack must match the declared function signature, avoiding stack imbalances.

- **Memory and Table Boundaries**: Ensures that memory and table sizes do not exceed predefined limits and that access to these regions remains within bounds.

#### Module Execution

During runtime, WAMR ensures that execution strictly conforms to the WebAssembly spec and maintains the security guarantees made at load time:

- **Memory Safety**: Memory access, both direct and indirect, is rigorously checked. WAMR prevents out-of-bounds access, helping mitigate common vulnerabilities like buffer overflows.

  - Implementation of **Boundary Checks**: WAMR can leverage either software boundary checks or hardware boundary checks. For software boundary checks, before each memory access, the address is validated to ensure it falls within the allowable bounds of the allocated memory. For hardware boundary checks, protection mechanisms such as `mmap`-based memory protection, where sections of memory can be made non-writable or non-executable to prevent invalid memory address access.
  
- Like previously mentioned, applications generally execute deterministically with **limited exceptions**, which can be handled in the runtime rather than simply crushing or causing undefined behavior. The exceptions that WAMR can handle include but are not limited to:

  - EXCE_UNREACHABLE: Triggered when unreachable code is executed.
  - EXCE_OUT_OF_MEMORY: Signaled when the runtime runs out of memory.
  - EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS: Raised when memory access goes out of bounds.
  - EXCE_INTEGER_OVERFLOW: Detects integer overflow during arithmetic operations.
  - EXCE_INTEGER_DIVIDE_BY_ZERO: Handles division by zero in integer operations.
  - EXCE_INVALID_CONVERSION_TO_INTEGER: Raised when an invalid conversion to an integer occurs.
  - EXCE_INVALID_FUNCTION_TYPE_INDEX: Triggered when an invalid function type index is accessed.
  - EXCE_INVALID_FUNCTION_INDEX: Signaled when an invalid function index is used.
  - EXCE_UNDEFINED_ELEMENT: Raised when accessing an undefined element.
  - EXCE_UNINITIALIZED_ELEMENT: Triggered when an uninitialized element is accessed.
  - EXCE_CALL_UNLINKED_IMPORT_FUNC: Handles calls to unlinked imported functions.
  - EXCE_NATIVE_STACK_OVERFLOW: Triggered when the native stack exceeds its limit.
  - EXCE_UNALIGNED_ATOMIC: Raised when an unaligned atomic operation is attempted.
  - EXCE_AUX_STACK_OVERFLOW: Signals that the auxiliary stack has overflowed.
  - EXCE_AUX_STACK_UNDERFLOW: Triggered when the auxiliary stack is underflowed.
  - EXCE_OUT_OF_BOUNDS_TABLE_ACCESS: Raised when accessing a table out of bounds.
  - EXCE_OPERAND_STACK_OVERFLOW: Signaled when the operand stack overflows.

These features, combined with the robust validation and execution checks, ensure that WAMR achieves comprehensive security for running WebAssembly modules.

### Extra Enhancements on Security

WAMR goes beyond the standard WebAssembly security features by offering additional mechanisms to enhance the security of applications, particularly in the areas of native API access control and hardware-based security.

#### Native API Export Control

WAMR allows WebAssembly applications to interact with the host environment through **exported native APIs**. However, unrestricted access to these APIs can introduce security risks, such as unauthorized system calls or resource manipulation. To mitigate these risks, WAMR implements a **fine-grained access control** mechanism for native APIs:

- **Restricted API Access**: Developers can explicitly define which native APIs are exposed to WebAssembly modules, limiting the surface area for potential misuse. This allows for precise control over which system resources (e.g., file system, networking, I/O devices) can be accessed by a module.

- **Custom API Policies**: WAMR supports customizable policies, enabling developers to set permissions and constraints on how WebAssembly modules can interact with native APIs. This is particularly useful for sandboxing untrusted code while still allowing necessary functionality under controlled conditions.

- **API Call Validation**: All calls to native APIs are validated at runtime to ensure that they conform to the defined policies, preventing unauthorized or malicious API usage.

#### Intel SGX Remote Attestation

WAMR enhances security for trusted execution environments (TEEs) through its support for **Intel Software Guard Extensions (SGX)**, which provides hardware-level security features, including **remote attestation**:

- **SGX Integration**: WAMR can run WebAssembly modules within an SGX enclave, a protected area of execution that provides isolation from the rest of the system. This ensures that even if the host machine is compromised, the code and data within the enclave remain secure.

- **Remote Attestation**: WAMR supports SGX remote attestation, allowing remote parties to verify that a WebAssembly module is running inside a genuine SGX enclave. This involves generating and sending an **attestation report**, which proves the authenticity of the enclave and the integrity of the code running inside it.

  - **Endorsement of Trusted Execution**: The attestation process ensures that the WebAssembly module and its execution environment have not been tampered with, providing assurance to remote users that the module is running in a secure, trusted state.

- **Sealing and Unsealing**: WAMR supports SGX's data sealing features, enabling WebAssembly modules to securely store sensitive data on disk. Sealed data can only be accessed by the same enclave in future executions, protecting it from unauthorized access even if the host system is compromised.

These features enhance WAMR’s security, making it suitable for use cases that require both flexible native API access and strong hardware-backed guarantees of code integrity and confidentiality.
