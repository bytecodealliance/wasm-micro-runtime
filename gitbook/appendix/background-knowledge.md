# Some background knowledge

In this section, we aggregate some basic background knowledge and jargon in our project field. This section could be served as a refresher for those who have left academia for a while and cannot fully recall all the weary details and exact meaning of jargon in the Compiler course. Also, it would be an great primer for those who did not take such a course and are interested in such a field(and, of course, our fantastic WAMR project).

We think providing such a section would be nice so that you do not have to Google around. If there is anything you find inaccurate, you think should be included, or even better, you have something for us that would take this section to the next level, do feel free to reach out to us on [GitHub](https://github.com/bytecodealliance/wasm-micro-runtime)!

Without further ado, let us dive right into our exciting recitation/learning journey!

## 1. Compiler

### 1.1 What is Compiler?

Strictly speaking(formal definition you usually find in textbooks), the compiler is a special computer program, a system program(serves as a platform for other software), to be more precise. It takes a source program as input and outputs a target program as a result. The source program is written in the source programming language, and usually, it is a high-level programming language such as C/C++, Java, Rust, and so on. The target program is written in a target programming language would be a low-level programming language like assembly. Take C/C++ as an example, the input for GCC compiler(component) is a C/C++ translation unit(a source file along with any header it used), and the output is platform-dependent assembly code.

However, in our daily life, what we usually mean when we refer to the word compiler is the compiler toolchain, which comprises a compiler, assembler, and linker. The assembler is in charge of translating the compiled translation unit(object file) from assembly to truly machine-readable machine code. The linker is used to link all the parts of the program(object files) into one executable file. Together, they can translate our human-readable source code(potentially many files) into a program that can run on the machine.

For now, we will mainly focus on the more strict definition because I think the concept and algorithm compiler use more closely pertain to our WAMR project (Also, due to the writer's limited knowledge regarding the other two parts).

<!-- TODO: graph -->

### 1.2 Structure and algorithm involved

<!-- TODO: graph -->

Since we alright know what a compiler is, now let's learn more details about compiler. First, let's talk about the structure of the compiler and the algorithm related to each part. Typically, the compiler consists of three parts, Front End, Optimizer and Back End:

- Front End: in some sense, this part is more "mature." The theory involved and actual implementation is more or less stable nowadays. Its primary purpose is to gather textual information from source-language programs and understand it syntactically and semantically. After that, it encodes the knowledge it has into Intermediate Representation. The theory behind Front End is formal language theory(Scanners & Parsers) and lattice theory(Elaboration for type checking).

- Optimizer: as the name suggests, the Optimizer's goal is to optimize our program's performance. Clever readers may be conscious of the difficulty when they hear about the word "optimize". Indeed, the Optimizer is very challenging to design and implement since it's a vital part of compiler infrastructure and imposes a heavy performance impact. It analyses the input IR and transforms it into definitive IR, usually through multiple passes, gradually accumulating knowledge of the program and applying a better(hopefully) transformation to it. The output(definitive IR) is semantically equivalent to the input IR to preserve the original meaning of the program we are compiling. The theories and algorithms that could be used for Optimizer are too many to list here. Here are some examples: Number theory, some graph algorithms for static analysis, and fixed-point algorithms for data-flow analysis. It's still an active field that attracts many people to research.

- Back End: the Back End is in charge of mapping programs (in IR form) to low-level code forms that can run on the target machine. Usually, there is more than one Back End, so the compiler is portable for different platforms (ISA). Its main functionality includes instruction selection, register allocation, and instruction scheduling, in which many algorithms are applied, like heuristic search, graph coloring, and some dynamic programming. Like Optimizer, the Back End has many open problems to tackle and also is a field many people hold great interest in.

## 2. Interpreter

### 2.1 What is Interpreter?

The interpreter is also a system computer program. Like the compiler, it takes a source program as input; but instead of outputting a target program, it directly executes the program line by line and returns the corresponding results. One thing worth noting is that it's not uncommon for an interpreter to adapt widely used techniques in the compilers to improve its performance. Sometimes they are even used together.

Based on the levels of the source language(high or low) and compilation strategies, the interpreters can be divided into several different categories. Let's look at them in more detail in the following section.

### 2.2  Technique and jargon in Interpreter

- Bytecode:

  The bytecode is a kind of low-level programming language in a very highly optimized and compact format. It could be the target language for the compiler. Because the instruction-like bytecode can be executed line by line in an interpreter on any platform, regardless of what hardware-related ISA it uses, it is also called p-code(portable). One example of bytecode you may be familiar with is Java bytecode.
  
- Ahead-of-time(AOT) and Just-in-time(JIT) compilation:

  - AOT: as the name suggests, the AOT compilation means that the compilation happens before the program run time. Normally the target language after  AOT compilation is some low-level machine code or bytecode. Then the compiled code can be executed by either a process VM or a normal computer.

  - JIT: just in time compilation is a technique widely adapted by the interpreter to improve its performance. It detects the heavily used code section when interpreting the program and compiles them into more efficient machine code. When that code section is called again, the machine code is executed rather than having the bytecode interpreted.

## 3. Virtual machines

When it comes to the word "Virtual Machines," we usually would remember or refer to is that System virtual machines managed by hypervisors such as KVM, VirtualBox, or VMware. We often use them as a substitute for real computers to resolve dependency or compatibility issues for courses or daily work.

But there is also another type of virtual machine you may have heard of(even you may get really confused at first) and related more closely to the field where our project is in. That is process (application) virtual machines, which provide an environment independent of hardware, aiming to run computer programs written in a certain language. Take JVM as an example, it provides an environment for Java bytecode to execute across many platforms.

## 4. Runtime system

It's rather a vague term that is really difficult to explain or understand. To make things worse, when people sometimes refer to it as runtime, it's easily confused with compilation runtime, runtime library. The runtime system is an infrastructure that participates in the creation and running of our program. Typically, the components are the execution environment(Application VM maybe) to provide a place for the program to run, and the compiler front end or/and compiler back end to do the necessary analysis, transformation(from source code to bytecode) and optimization.
