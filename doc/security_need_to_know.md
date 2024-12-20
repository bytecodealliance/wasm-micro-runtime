# About security issues

This document aims to explain the process of identifying a security issue and the steps for managing a security issue.

## identifying a security issue

It is commonly stated that a security issue is an issue that:
- Exposes sensitive information to unauthorized parties.
- Allows unauthorized modification of data or system state.
- Affects the availability of the system or its services.
- Permits unauthorized access to the system.
- Enables users to perform actions they should not be able to.
- Allows users to deny actions they have performed.

Given that WASI is a set of Capability-based APIs, all unauthorized actions are not supposed to happen. Most of the above security concerns can be alleviated. What remains for us is to ensure that Wasm modules ' execution is secure. In other words, do not compromise the sandbox. Unless it is explicitly disabled beforehand.

Thus, we share most of the criteria for judging security issues with [the Bytecode Alliance](https://github.com/bytecodealliance/rfcs/blob/main/accepted/what-is-considered-a-security-bug.md#definition).

>[!NOTE]
> keep updating this document as the project evolves.

## managing a security issue

Before reporting an issue, particularly one related to crashing, consult [the cheat sheet](), *Report a security vulnerability*  if it qualifies.

Upon receiving an issue, thoroughly review [the cheat sheet] to assess and *Report a security vulnerability* if the issue is indeed a security vulnerability .

Once a security issue is confirmed, please refer to [the runbook](https://github.com/bytecodealliance/rfcs/blob/main/accepted/vulnerability-response-runbook.md) for the subsequent steps to take.
