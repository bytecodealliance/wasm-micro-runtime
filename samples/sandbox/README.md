Process-based sandbox experiment
================================

*WARNING*: This is not a complete implementation and almost certainly has security issues.

This is a prototype of loading a library in a sandboxed child process.
Parts of this are intended to be useable for process-based sandboxing of legacy libraries in Verona programs.

For each sandbox instance, the parent process creates two anonymous shared memory objects.
One object is large and will be used for the sandbox's heap.
The large object is mapped at the same address in both the parent and a child process, allowing intra-sandbox pointers to have the same values in both.
This is mapped read-write in both processes.

The smaller object is a single page, used for the snmalloc page map that covers the large region.
This is mapped read-write in the parent over the region of the pagemap that contains metadata for the large region.
It is mapped read-only in the child.

The parent has an allocator associated with the sandbox that allows allocation in the sandbox's heap.
The parent allocator's message queue is in the shared region, but the rest of the state is not.
When the allocator in the parent needs to update the pagemap, it does so directly.
When an allocator in the child needs to update the parent, it communicates the update via a pipe to the parent.
The parent then validates the requested update and performs the write.

When the child starts, it first maps the shared region and initialises a version of snmalloc as its malloc implementation.
The child process' snmalloc uses a custom PAL that is backed by the shared memory region and a custom PageMap to write updates.
On systems with Capsicum support, it then enters capability mode, at which point it has no access to any global namespaces other than those explicitly passed by the parent.
The child then closes the file descriptors for the shared mapping and opens the requested library.
The library initialisation code is the first untrusted code that runs in the child and it does so only after the child has dropped all OS rights.

Finally, the child process enters a run loop waiting for messages from the parent.

Note that, for this to be efficient, the OS must implement lazy commit so that allocating a large (e.g. 1GiB) shared memory region does not consume 1GiB of physical memory or swap unless it is actually used.

Currently, the in-memory RPC mechanism used to invoke methods in the child is very high latency.
This may not be a problem for Verona, where foreign calls are likely to be wrapped in `when` clauses, which can batch multiple operations within the library.
The asynchronous operation of `when` clauses hides latency, avoiding the blocking operations in the C++ proof-of-concept.
This overhead could be significantly reduced on an OS that supported Spring / Solaris Doors.
