# Lemon Roadmap

Lemon (`lem`) is a C-like systems language with slices, `Result`/`?`, `defer`, methods, generics and an ergonomic POSIX layer. This file tracks how the compiler, written in **C17**, gets built: milestone by milestone, each one ending in a program that compiles and runs.

The language itself is specified in the Lemon Language Design doc. This roadmap is the order of work, not the spec. When implementation and spec disagree, write it down in `NOTES.md` and fix one of them.

**Status legend:** ⬜ not started · 🟨 in progress · ✅ done · ⏸ optional / deferred

**Estimated total:** 10–13 months at roughly 8–10 hours a week.

---

## Contents

1. [Principles](#principles)
2. [Repository layout](#repository-layout)
3. [Milestones at a glance](#milestones-at-a-glance)
4. [Milestones in detail](#milestones-in-detail)
5. [Testing strategy](#testing-strategy)
6. [Definition of done](#definition-of-done)
7. [Out of scope for 1.0](#out-of-scope-for-10)
8. [References](#references)

---

## Principles

1. **Walking skeleton first.** `.lem` → C → executable works end to end in M1, before any interesting feature. Every later milestone widens a pipeline that already runs.
2. **A feature lands only with a test that needs it.** The design doc is the destination, not a checklist to implement top to bottom.
3. **Golden tests from day one.** Every bug fix adds a test that failed before the fix.
4. **Arenas for everything the compiler allocates.** AST nodes, types, symbols, strings and LIR live in arenas released at exit. No `free()` in the front end.
5. **Sanitizers on in every debug build of the compiler:** `-g -fsanitize=address,undefined -Wall -Wextra -Werror`.
6. **Errors are values.** Diagnostics are collected, the parser recovers at `;` and `}`, and one run reports many errors. `ICE()` aborts only on internal bugs.
7. **Source locations everywhere.** Every token, AST node and LIR instruction carries `SrcLoc`; every error shows the line with a caret.
8. **Plain C, plain build.** C17, one Makefile, one header per subsystem, no macro tricks beyond the growable-array helpers.

---


## Milestones at a glance

| # | Milestone | Tag | Est. | Exit program | Status |
|---|---|---|---|---|---|
| M0 | C toolkit and front-end foundations | — | 3–4 wk | calculator with caret errors | ⬜ |
| M1 | Walking skeleton: `.lem` → C → binary | `v0.0.1` | 3–4 wk | recursive `fib` | ⬜ |
| M2 | Types and type checking | `v0.0.2` | 4–5 wk | linked list with raw pointers | ⬜ |
| M3 | Modules and the build tool | `v0.0.3` | 2–3 wk | 3-module project, one command | ⬜ |
| M4 | C header import with libclang | `v0.1.0` | 3 wk | calls `printf`, `open`, zlib | ⬜ |
| M5 | LIR, slices, `defer`, methods, named args | `v0.1.1` | 4–5 wk | `IntStack` with `defer` | ⬜ |
| M6 | Generics and constraints | `v0.2.0` | 5–6 wk | `Stack<T>`, `List<T>` | ⬜ |
| M7 | Tagged unions, `Result`, `?`, operators, casts | `v0.2.1` | 4 wk | `?`, exhaustive `switch`, `maximum` | ⬜ |
| M8 | Preprocessor, `#debug`, `#test` | `v0.3.0` | 2–3 wk | `lem test` runs everything | ⬜ |
| M9 | Standard library and POSIX layer | `v0.4.0` | 5–6 wk | `mmap` line counter, child process | ⬜ |
| M10 | Dogfooding and tooling | `v0.5.0` | 4 wk | 3 real tools, first public release | ⬜ |
| M11 | Native backend: LLVM C API or QBE | `v0.6.0` | 6–8 wk | test suite passes on both backends | ⏸ |
| M12 | Self-hosting | `v1.0.0`? | open | compiler built by itself | ⏸ |

```
M0 → M1 → M2 → M3 → M4        a compiler in C that works        (~4–5 months)
M5 → M6 → M7 → M8             the features that make it Lemon   (~4 months)
M9 → M10 → (M11) → (M12)      usable by other people            (~3–4 months)
```

M8 may move right after M5 if `#test` would help earlier.

---

## Milestones in detail

### M0 · C toolkit and front-end foundations ⬜

**Goal:** the C building blocks every later milestone uses, plus a throwaway language to practise lexing, Pratt parsing and diagnostics.

**Learn:** *Crafting Interpreters* ch. 4–8 and Part III ch. 14, 19, 20 · Hanson, *C Interfaces and Implementations* (Arena, Atom, Table) · Fleury, *Untangling Lifetimes* · matklad, *Simple but Powerful Pratt Parsing*.

**Tasks**

- [x] Makefile with `debug` (sanitizers) and `release` targets
- [x] `arena.[ch]`: `arena_alloc`, `arena_strdup`, `arena_reset`, `arena_destroy`. Single-threaded, one fixed `mmap` block
- [x] `dynamic_array.h`: `GROW_ARRAY`/`FREE_ARRAY` for manually resized buffers, `DA_PUSH`/`DA_FREE` for `count`/`capacity`/`items` structs. This is the vec; no separate `vec.h`, no `stb_ds.h`
- [ ] `strbuf.[ch]`: `append`, `appendf`, `cstr` — a `DA_PUSH` byte array plus `vsnprintf`
- [x] `map.[ch]`: open addressing, string and pointer keys, iteration, and `intern(const char*, len)` returning a unique pointer (interning is a lookup plus `arena_strdup`, not its own subsystem)
- [ ] `diag.[ch]`: `SrcLoc`, `error_at`, `warning_at`, caret printing, error count, `ICE()`; a failed `arena_alloc` becomes `ICE()` here
- [ ] Unit tests for `src/utils` in `tests/base/` under ASan and UBSan, using the vendored single-header [greatest](https://github.com/silentbicycle/greatest) (arena and dynamic array done in `utils_test.c`)
- [ ] Throwaway calculator: lexer → Pratt parser → tagged-union AST → evaluator (`let`, `if`)
- [ ] `tests/run.sh` skeleton

**Exit criteria**

- [ ] A typo in `1 + 2 * (3 - x)` reports the exact line and column with a caret
- [ ] `tests/base` passes under ASan and UBSan with no leaks or errors

---

### M1 · Walking skeleton: `.lem` → C → binary ⬜ `v0.0.1`

**Goal:** a real Lemon front end for a tiny subset, emitting C and producing an executable.

**Learn:** Sandler, *Writing a C Compiler* ch. 1–4 · chibicc, first ~40 commits (`tokenize.c`, `parse.c`, `codegen.c`).

**Tasks**

- [ ] Lexer: all keywords, operators, `0b` literals, `_` separators, char/string literals, comments; identifiers interned
- [ ] Parser: functions, `int` locals, arithmetic, comparisons, logical ops, `if`/`else`, `while`, `for`, `return`, calls
- [ ] AST as tagged unions in the arena
- [ ] `emit_c.c`: AST → C text in a `StrBuf`, with `#line` directives
- [ ] Driver: `lem build file.lem` writes C, runs `cc` via `fork`/`execvp` (or `posix_spawn`), `waitpid`, forwards errors
- [ ] `lem run file.lem`
- [ ] `tests/ok/*.lem` + `.out` harness

**Exit criteria**

- [ ] Recursive `fib` prints correct values
- [ ] ≥ 20 tests in `tests/ok` pass, clean under sanitizers

---

### M2 · Types and type checking ⬜ `v0.0.2`

**Goal:** a real static type system matching C's rules plus Lemon's integer names.

**Learn:** Sandler, chapters on types, pointers, arrays, structures · *Crafting Interpreters* ch. 11 · C23 draft for conversion rules.

**Tasks**

- [ ] `Type` tagged union: `u8`–`u64`, `i8`–`i64`, `usize`/`isize`, `f32`/`f64`, `bool`, `char`, `byte`, pointers, arrays, structs, function types
- [ ] Canonical types (`Map`-backed constructors) so equality is pointer comparison
- [ ] Scopes as a stack of `Map`s; symbols with kind, type, `SrcLoc`
- [ ] Two-pass checking: collect top-level declarations, then check bodies (order-independent)
- [ ] Structs, field access, `->`, compound literals, designated initialisers
- [ ] Usual arithmetic conversions, implicit-narrowing warnings, explicit casts
- [ ] `auto` (locals only), zero-initialisation, `uninit`
- [ ] `typedef` and non-generic `using`
- [ ] Literals: `0b`, `_`, octal
- [ ] `tests/err/` harness: expected error text and location

**Exit criteria**

- [ ] Linked list with raw structs, pointers and `malloc` compiles and runs
- [ ] ≥ 15 `tests/err` cases produce exactly the expected diagnostics

---

### M3 · Modules and the build tool ⬜ `v0.0.3`

**Goal:** headerless modules and a project-level build.

**Learn:** Go spec (packages, imports) · c3c module resolution · TLPI ch. 18 (directories).

**Tasks**

- [ ] `module a.b;` declaration, checked against file path
- [ ] `import a.b;`, `import a.b as x;`, qualified and unqualified names, ambiguity errors
- [ ] `static` = module-private; everything else exported
- [ ] Module-level `const` as compile-time constants (array sizes)
- [ ] Import graph; declaration collection tolerant of cycles; `const` cycles rejected
- [ ] One `.c` per module + generated header in `build/include/`
- [ ] `lem.toml` with tomlc99: `[package]`, `[link]`, `[build]`
- [ ] `lem build` / `lem run` on a project directory; walk `src/` with `opendir`/`readdir`
- [ ] Optional: compile modules in parallel child processes

**Exit criteria**

- [ ] 3-module project (`main`, `ds.stack`, `util.math`) builds and runs with one command
- [ ] A C program can include a generated header and call a Lemon function

---

### M4 · C header import with libclang ⬜ `v0.1.0`

**Goal:** `import <header.h>` gives typed access to any C library.

**Learn:** Clang *Libclang tutorial* · Zig docs, C section.

**Tasks**

- [ ] Link libclang (`llvm-config --cflags --ldflags`)
- [ ] Parse headers; convert functions, structs, unions, enums, typedefs, globals
- [ ] Object-like macros via a synthetic `const __auto_type lem_m = MACRO;` + evaluation
- [ ] Function-like macros recorded as unchecked macro calls, emitted verbatim
- [ ] C struct tags keep their own namespace (`struct stat` vs `stat`)
- [ ] `c.` prefix for explicit C calls
- [ ] Emit real `#include`s in generated C
- [ ] Header cache per build
- [ ] `[link] libs` and `pkg-config` in `lem.toml`

**Exit criteria**

- [ ] A Lemon program calls `printf`, `malloc`, `open` and `S_ISDIR`
- [ ] A program links against zlib and prints `zlibVersion()`

---

### M5 · LIR, slices, `defer`, methods, named arguments ⬜ `v0.1.1`

**Goal:** the core ergonomics, on top of a proper intermediate representation.

**Learn:** Sandler, TACKY chapters · *Engineering a Compiler*, IR chapter · Go spec and Zig docs on `defer`/`errdefer`.

**Tasks**

- [ ] `lir.[ch]`: per-function instruction arrays, temporaries, labels, jumps
- [ ] `lower.c`: AST → LIR; `emit_c.c` switched to read LIR
- [ ] `defer` (block-scoped, LIFO) on every exit path; return value computed before cleanups
- [ ] Slices: `T[]` layout, `a[lo:hi]`, `p[0:n]`, `.ptr`/`.len`, `const T[]`, `byte[]` conversion
- [ ] `for (x : s)` and `for (T* x : s)`
- [ ] Bounds-check instruction, present in debug/test, absent in release
- [ ] Methods: `self` receivers, auto address-of, `->` calls, associated functions, extension methods with visibility rules
- [ ] Named and default arguments, defaults referencing earlier parameters, call-site rewriting
- [ ] Build modes: `--release`; signed-overflow traps in debug (`__builtin_*_overflow`), wraps in release

**Exit criteria**

- [ ] Non-generic `IntStack` with `push`/`pop`/`defer` cleanup passes in debug and release
- [ ] Out-of-bounds slice index traps in debug with index, length and location

---

### M6 · Generics and constraints ⬜ `v0.2.0`

**Goal:** monomorphized generics with structural constraints. The hardest milestone.

**Learn:** rustc dev guide, *Monomorphization* · Go *Type Parameters Proposal* · c3c generic modules.

**Tasks**

- [ ] Parse `<T>` on functions, `struct Name<T>`, generic methods
- [ ] Check generic bodies with abstract `T`
- [ ] Instance cache: key = declaration pointer + canonical `Type*` arguments
- [ ] AST substitution and re-check per instance
- [ ] Mangling (`Stack_u32__push`); emit only used instances
- [ ] Type-argument inference for calls
- [ ] `constraint Name<T> { methods }`, union constraints, `&` combinations
- [ ] Structural satisfaction; built-in `cmp`/`eq`/`hash` on primitives; floats not `Comparable`
- [ ] Method-level constraints (`<Equatable T> List<T>.contains`)
- [ ] Generic `using` aliases (`using Grid<T> = ...`)
- [ ] One-line constraint failure messages

**Exit criteria**

- [ ] `Stack<T>` and `List<T>` from the design doc compile unchanged and pass their tests
- [ ] `maximum` works for `i32`, `str` and a user `Version` type

---

### M7 · Tagged unions, `Result`, `?`, operators, casts ⬜ `v0.2.1`

**Goal:** Lemon's error model and the remaining expression features.

**Learn:** *Mapping High Level Constructs to LLVM IR* (unions, structures) · Rust Reference, `?` operator.

**Tasks**

- [ ] Payload enums → `{tag, union}`; constructors with named fields
- [ ] `switch` with bindings, `case X(*p)`, exhaustiveness, fall-into-binding error
- [ ] `std/prelude.lem`: `Result<T>`, `Option<T>`, `Error`, their methods
- [ ] `?` lowering with `defer`/`errdefer` cleanups
- [ ] Must-use `Result`; `(void)` discard; allowed inside `defer`
- [ ] `main` returning `Result<void>` → exit code
- [ ] Operator overloading by method-name rewriting; non-overloadable list enforced
- [ ] Full overload resolution, including Lemon-over-C ties
- [ ] Casts to types with `from` → `T.from(x)`; only non-`Result` overloads allowed
- [ ] Typed variadics `...T` and `...any`

**Exit criteria**

- [ ] `parse_and_add` with `?`, `area(Shape)` with exhaustive `switch`, `maximum(Version, Version)` all pass
- [ ] `(str)argv[1]` works

---

### M8 · Preprocessor, `#debug`, `#test` ⬜ `v0.3.0`

**Goal:** file-scoped preprocessing and built-in testing.

**Learn:** GNU *The C Preprocessor* · chibicc `preprocess.c`.

**Tasks**

- [ ] Token-level preprocessor: `#define`, `#undef`, `#if`/`#ifdef`/`#elif`/`#else`/`#endif`, `#error`
- [ ] `#include` rejected with a hint
- [ ] `LEM_*` build constants; `[defines]` from `lem.toml`
- [ ] Imports inside `#if` resolved after preprocessing
- [ ] `#debug` blocks: declarations marked, calls and argument evaluation dropped in release, `void`-only rule, confinement rule
- [ ] `assert` behaves as a debug function
- [ ] `#test` blocks: every function is a test, no parameters, `void` or `Result<void>`
- [ ] `*.test.lem` files sharing the module's private scope
- [ ] Generated test runner: one `fork` per test, `waitpid`, summary
- [ ] `lem test [filter]`

**Exit criteria**

- [ ] `lem test` runs the Fibonacci, stack and list tests
- [ ] A deliberately crashing test fails alone without stopping the run

---

### M9 · Standard library and POSIX layer ⬜ `v0.4.0`

**Goal:** the USP: POSIX done right, written in Lemon.

**Learn:** TLPI ch. 4–5, 20–21, 24–27, 49 · APUE · Fleury on arenas.

**Tasks**

- [ ] `Error.from_errno()` with a generated static message table
- [ ] `<mem>`: `Allocator`, `heap`, `Arena`, `Pool<T>`, `scratch()`, `DebugAllocator` (guards, poisoning, leak report)
- [ ] `heap` is `DebugAllocator` in debug and test builds
- [ ] `<string>`: `str` methods, `String`, `copy_cstr`, `from` overloads
- [ ] `<io>`: `print`, `println`, `eprintln`, `print_to`, `{}` formatting
- [ ] `<unistd>`: `read`, `write`, `write_all`, `close`, `pipe`, `fork`, `execvp`, `dup2`, `unlink`, ...
- [ ] `<fcntl>`: `open`, `openat`
- [ ] `<sys/stat>`: `stat`, `fstat`, `mkdir` (`parents`, `exist_ok`), `chmod`, generated `Perm` enum (512 values) + `SETUID`/`SETGID`/`STICKY`
- [ ] `<sys/mman>`: `mmap`, `munmap`, `mprotect`
- [ ] `<sys/wait>`: `waitpid` returning `WaitStatus`
- [ ] `<dirent>`: `opendir`, `readdir`, `list_dir`
- [ ] Every wrapper follows the ten POSIX-layer rules
- [ ] Tests: `EINTR` retry (signal yourself), `ENAMETOOLONG`, short writes

**Exit criteria**

- [ ] `mmap` line counter and child-process examples run unchanged
- [ ] `DebugAllocator` reports a deliberate leak with its source line

---

### M10 · Dogfooding and tooling ⬜ `v0.5.0` (first public release)

**Goal:** prove the language on real programs and make it installable.

**Learn:** Czaplicki, *Compiler Errors for Humans* · *The Cargo Book*.

**Tasks**

- [ ] `tools/lgrep`: a `find`/`grep` clone
- [ ] `tools/supervise`: a process supervisor that restarts a crashing child
- [ ] `tools/lwc`: `wc`/`cat` replacement
- [ ] Fix every compiler bug and language annoyance the tools expose (log them in `NOTES.md`)
- [ ] Rework the five worst error messages
- [ ] `lem add` with Git dependencies, `lem.lock`
- [ ] `lem doc` from `///` comments
- [ ] `lem fmt` (basic)
- [ ] `lem init` (bin and `--lib`)
- [ ] `lem emit-c`
- [ ] Install script and README quick-start
- [ ] Comparison page: each tool vs C and Python, with line counts and timings

**Exit criteria**

- [ ] A fresh machine installs `lem`, clones a tool and builds it in under five minutes

---

### M11 · Native backend: LLVM C API or QBE ⏸ `v0.6.0`

**Goal:** a second backend, mainly for debugging quality (LLVM) or simplicity (QBE).

**Learn:** LLVM Kaleidoscope (ch. 1–4, 8–9) mapped to the C API · *Mapping High Level Constructs to LLVM IR* · System V x86-64 psABI ch. 3 · c3c's C ABI lowering · QBE IL docs, cproc, harec.

**Tasks**

- [ ] Choose LLVM-C or QBE; record the decision in `NOTES.md`
- [ ] LIR → backend for code without C calls
- [ ] `--backend=` flag and `[build] backend` in `lem.toml`
- [ ] (LLVM) DWARF via `LLVMDIBuilder*`: Lemon types for slices and tagged unions
- [ ] C calls: scalars and pointers first, then structs by value (own classification for LLVM; QBE handles it)
- [ ] Function-like macro shims: generated C wrappers compiled with `cc` and linked
- [ ] CI runs the full test suite on both backends

**Exit criteria**

- [ ] Every test passes on both backends
- [ ] (LLVM) `gdb` displays a `u32[]` as a slice

---

### M12 · Self-hosting ⏸

**Goal:** rewrite the compiler in Lemon.

**Tasks**

- [ ] Port `src/base` to Lemon (`Arena`, `Vec<T>`, `Map<K, V>`, `String`)
- [ ] Port the lexer, then the parser, then checking and lowering
- [ ] Stage 1: C compiler builds the Lemon compiler; stage 2: the Lemon compiler builds itself; outputs match

---

## Testing strategy

| Kind | Location | How it runs | Added from |
|---|---|---|---|
| Toolkit unit tests | `tests/base/*.c` | `make test`, compiled with sanitizers, one binary per file | M0 |
| Run tests | `tests/ok/NAME.lem` + `NAME.out` | compile, run, diff stdout (and exit code if `NAME.code` exists) | M1 |
| Error tests | `tests/err/NAME.lem` | comments `// error: LINE:COL message` must match diagnostics exactly | M2 |
| In-language tests | `#test` blocks, `tests/std/` | `lem test` | M8 |
| Example programs | `examples/` | built and run in CI | M6+ |
| Backend parity | all of the above | run once per backend; outputs must match | M11 |

Rules:

- `tests/run.sh` prints a pass/fail count and the first diff of each failure.
- Run it before every commit; CI runs it on every push (Linux, and macOS once M9 starts).
- Every bug fix adds a test that failed before the fix.
- The compiler itself is always tested in its sanitizer build.

---

## Definition of done

A change is done when:

- [ ] It has at least one new test in `tests/ok`, `tests/err` or a `#test` block
- [ ] `tests/run.sh` passes with the sanitizer build of the compiler
- [ ] Diagnostics it adds point at the right line and column
- [ ] Any spec question it raised is recorded in `NOTES.md`
- [ ] The milestone checklist above is updated

A milestone is done when every exit criterion is ticked and its tag is pushed.

---

## Out of scope for 1.0

Classes and inheritance · exceptions · destructors/RAII · garbage collection · borrow checking · capturing closures · async/await · compile-time code execution · hygienic macros · template specialisation · runtime reflection beyond `any` · Windows · a central package registry.

---

## References

**Core**

- Robert Nystrom, [Crafting Interpreters](https://craftinginterpreters.com) (Part III is in C)
- Nora Sandler, [Writing a C Compiler](https://nostarch.com/writing-c-compiler) (No Starch, 2024)
- David Hanson, *C Interfaces and Implementations*
- Michael Kerrisk, [The Linux Programming Interface](https://man7.org/tlpi/)

**By milestone**

- M0: Ryan Fleury, [Untangling Lifetimes: The Arena Allocator](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator) · Alex Kladov, [Simple but Powerful Pratt Parsing](https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html) · [stb_ds.h](https://github.com/nothings/stb)
- M1–M2: [chibicc](https://github.com/rui314/chibicc) · C23 standard draft (ISO WG14)
- M3: [The Go Programming Language Specification](https://go.dev/ref/spec) · [tomlc99](https://github.com/cktan/tomlc99) · [c3c](https://github.com/c3lang/c3c)
- M4: [Libclang tutorial](https://clang.llvm.org/docs/LibClang.html) · [Zig documentation](https://ziglang.org/documentation/master/)
- M5: Keith Cooper and Linda Torczon, *Engineering a Compiler*
- M6: [rustc dev guide: Monomorphization](https://rustc-dev-guide.rust-lang.org/backend/monomorph.html) · [Go Type Parameters Proposal](https://go.googlesource.com/proposal/+/master/design/43651-type-parameters.md)
- M7: [Mapping High Level Constructs to LLVM IR](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/) · [The Rust Reference](https://doc.rust-lang.org/reference/)
- M8: [The C Preprocessor (GNU)](https://gcc.gnu.org/onlinedocs/cpp/)
- M9: W. Richard Stevens and Stephen Rago, *Advanced Programming in the UNIX Environment*
- M10: Evan Czaplicki, [Compiler Errors for Humans](https://elm-lang.org/news/compiler-errors-for-humans) · [The Cargo Book](https://doc.rust-lang.org/cargo/)
- M11: [Kaleidoscope tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) · [LLVM C API](https://llvm.org/doxygen/group__LLVMC.html) · [LLVM Language Reference](https://llvm.org/docs/LangRef.html) · [System V x86-64 psABI](https://gitlab.com/x86-psABIs/x86-64-ABI) · [QBE](https://c9x.me/compile/) · [cproc](https://git.sr.ht/~mcf/cproc) · [Hare](https://harelang.org)
