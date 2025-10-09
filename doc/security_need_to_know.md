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

Given that WASI is a set of Capability-based APIs, all unauthorized actions are not supposed to happen. Most of the above security concerns can be alleviated. What remains for us is to ensure that the execution of Wasm modules is secure. In other words, do not compromise the sandbox. Unless it is explicitly disabled beforehand.

### Is this bug considered a security vulnerability?

#### For someone who finds a problem

if a bug **results in crash or hang**, please treat it as a security problem and report it to a security advisor. The maintainer will look into it and change its category if needed. It is better safe than sorry.

If the author of an issue(results in crash or hang) can go through the following checklist and answer all questions with "No", it is fine to mark it as a regular bug. If not, please report it as a security issue.

---

#### For those maintainers

please use the following guidelines to determine if a bug or advisory is a security issue:

Only bugs that affect [tier A platforms or features](./tiered_support.md) should be considered.

Actions that differ from Wasm rules (like calculating wrong values) are not seen as security issues as long as they stay within the sandbox.

By default, native APIs and CLIs are following the principle of **caller guarantee**. If the caller provides incorrect parameters or users input malformed options, it is not a security issue. For example, if a user passes an invalid file descriptor to `fd_read`, it is not a security issue.

.wasm are not trusted. Malformed .wasm files should be handled gracefully. If a .wasm file causes a runtime crash or hang, it is a security issue. On the other hand, it's expected that aot runtime alone doesn't provide the same guarantee. So user-crafted .aot can cause anything, including crashes or hangs. They are not considered security issues.

A denial-of-service (DoS) attack is a cyberattack that aims to make a computer or network resource unavailable to its users. If the service (runtime in this case) can recover and start another module or run another function within the same instance, it is not considered unavailable, and thus not a Denial of Service (DoS).

Another type of execution problem we usually do not classify as a security one is if it is caused by an infinite loop or incorrect recursive function call chain.

### When a maintainer identify a problem that should be classified as a security vulnerability

Once a maintainer realizes an issue or PR describes a real or possible security vulnerability, act quickly to minimize exposure. Do not share technical details publicly on the issue or PR anymore. Maintainers should:

- Close or edit the public discussion. Thank the person who reported it and explain that security-related issues should go through the Security Advisory process. Close the public issue or pull request as soon as possible to prevent further public sharing. If details have already been shared, consider editing or asking GitHub staff to remove sensitive content.

- Create a Security Advisory. Invite the reporter to join as a collaborator or reporter. If the reporter is uncomfortable using GitHub Security Advisories, offer another private communication method, such as email.

- Follow the guidelines in [the security issue runbook](./security_issue_runbook.md) for the next steps.

## reporting a security issue

Follow the [same guidelines](https://bytecodealliance.org/security) as other projects within the Bytecode Alliance.

## managing a security issue

Once a security issue is confirmed, please refer to [the runbook](./security_issue_runbook.md) for the subsequent steps to take.
