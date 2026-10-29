# What this particular folder contains

C language has a very minimal standard library, and C programmers often need to make their own utilities for their project.
This folder contains some of those utilities

### Arena - (arena.h and arena.c)

Contains the code of an arena allocator. An arena allocator stores all the objects in a continuous chunk of memory which is not freed until the program finishes execution
In the final cleanup, we clean the entire continuous chunk of memory.
This is useful in situations like the `AST`, where we need the whole code to stay until the very end. It also is faster that the traditional `malloc` and leads to less memory fragmentation too

The compiler is a single-threaded process, so the arena takes no lock. M3 compiles modules in parallel *child processes*, which get their own address space and their own arena, and if threads ever arrive each one should get its own arena rather than contend on a shared one.

It is one fixed `mmap` block and `arena_alloc` returns `NULL` when that block is full. It is as I will reserve far more than expected: untouched pages cost no memory on Linux due to lazy allocation. 


### Dynamic Array - (dynamic_array.h and dynamic_array.c)

I have implemented a very basic dynamic array, inspired from the book `Crafting Interpreters`
The array is stored in the normal heap (`malloc`-ed), and `reallocate` is the single helper every macro goes through
The use of macros is to simplify the API throughout the project

There are two styles, for the two things a compiler needs:

- `GROW_ARRAY` / `FREE_ARRAY` resize a buffer you are managing yourself, like a hash table rebuilding its buckets.
- `DA_PUSH` / `DA_FREE` work on any struct holding `count`, `capacity` and an `items` pointer, zero-initialised as `Tokens t = {0};`. The element type is read off `items`, so the macro needs no type argument. This is the growable array the roadmap calls `vec` — there is no separate `vec.h`.

### Common - (common.h)

Contains a simple implementation of a logger (too simple to be honest)
and defines `DEBUG_MODE` 
also includes a lot of standard libraries to be used throughout the project
