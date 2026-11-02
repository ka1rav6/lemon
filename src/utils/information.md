# What this particular folder contains

C language has a very minimal standard library, and C programmers often need to make their own utilities for their project.
This folder contains some of those utilities

### Arena - (arena.h and arena.c)

**Used for:** everything the front end allocates and keeps to the end:  AST nodes, `Type`s, symbols, interned names, LIR : all dropped in one `arena_destroy` at exit.

Contains the code of an arena allocator. An arena allocator stores all the objects in a continuous chunk of memory which is not freed until the program finishes execution
In the final cleanup, we clean the entire continuous chunk of memory.
This is useful in situations like the `AST`, where we need the whole code to stay until the very end. It also is faster that the traditional `malloc` and leads to less memory fragmentation too

It is one fixed `mmap` block and `arena_alloc` returns `NULL` when that block is full. It is as I will reserve far more than expected: untouched pages cost no memory on Linux due to lazy allocation. 


### Dynamic Array - (dynamic_array.h and dynamic_array.c)

**Used for:** every growable list : tokens, AST child lists, diagnostics, per-function LIR arrays, `StrBuf` bytes (`DA_PUSH`); and `Map`'s own bucket array (`GROW_ARRAY`).

I have implemented a very basic dynamic array, inspired from the book `Crafting Interpreters`
The array is stored in the normal heap (`malloc`-ed), and `reallocate` is the single helper every macro goes through
The use of macros is to simplify the API throughout the project

There are two styles, for the two things a compiler needs:

- `GROW_ARRAY` / `FREE_ARRAY` resize a buffer you are managing yourself, like a hash table rebuilding its buckets.
- `DA_PUSH` / `DA_FREE` work on any struct holding `count`, `capacity` and an `items` pointer, zero-initialised as `Tokens t = {0};`. The element type is read off `items`, so the macro needs no type argument. This is the growable array the roadmap calls `vec` : there is no separate `vec.h`.

Both take the allocator as their last argument: `NULL` is the heap, and an `Arena*` puts the elements in that arena instead. The arena case exists for lists that outlive the function building them : the statements of a `Block`, the arguments of a `Call` : because principle 4 says nothing in the front end should have to walk the finished AST freeing them; `DA_FREE` with an arena only zeroes the struct. Pass the same allocator to both macros or `DA_FREE` will `free()` memory the heap never handed out. Growing inside an arena copies to a fresh block and strands the old one, so N elements cost about 2N, and anything scratch is still better off on the heap. `GROW_ARRAY` / `FREE_ARRAY` are heap-only on purpose: `Map` frees its old bucket array on every rehash, which an arena cannot do.

### String Buffer - (strbuf.h and strbuf.c)

**Used for:** the generated C text in `emit_c.c` (M1), and every diagnostic message built up before it is printed.

A growable `char` array : a `DA_PUSH` buffer with a header, so `strbuf_create(NULL)` gives a heap buffer you `strbuf_destory`, and `strbuf_create(ar)` puts both the header and the characters in the arena and the destroy becomes a no-op.

`count` is the number of real characters and never includes a terminator. `cstr` writes the `'\0'` just past them and hands back the pointer, so appending afterwards simply overwrites it and the buffer is a valid C string whenever you ask for one.

`STR_APPEND` is one name for both `appendc` and `append`, dispatched with `_Generic` on the argument type. A `sizeof` test cannot do this : a ternary compiles both arms, so `appendc` would still be handed a `char*`.

### Map - (map.h and map.c)

**Used for:** interning identifiers in the lexer, then everything keyed on them : scopes (M2), canonical types (M2), module and import tables (M3), the libclang header cache (M4), the generic-instance cache (M6).

A simple implementation of a hashmap with open addressing.

Keys are `const char*` either way, and `map_init` decides what that pointer
means: `MAP_STR_KEYS` compares the characters, `MAP_PTR_KEYS` compares the
address. 

Most of the compiler wants the second one, because identifiers are
interned first and after that two names are the same name exactly when their
pointers match.

`map_intern(m, ar, s, len)` is that interning thing: a lookup, and on
a miss a copy of the slice into the arena. It takes a length because the lexer
hands over a slice of the source file, which has no NUL to stop at.

### Common - (common.h)

**Used for:** included by every file; `LOG` is throwaway tracing until `diag.[ch]` lands, and `DEBUG_MODE` is the flag the debug-only bounds checks and `#debug` blocks (M5, M8) will key off.

Contains a simple implementation of a logger (too simple to be honest)
and defines `DEBUG_MODE` 
also includes a lot of standard libraries to be used throughout the project
