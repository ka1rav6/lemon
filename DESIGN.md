# Lemon Language Design

Version 0.3 · Oct 26, 2026 · @Kairav

## 1. Overview

Lemon (short form and tool name: `lem`) is C with the boilerplate removed: same syntax, same memory model, same ABI, plus generics, methods, slices, `Result`, `defer`, named arguments and headerless modules. Its reason to exist is the **POSIX layer**: every common system call wrapped with `Result` errors, slices instead of pointer-plus-length, and sensible defaults, while the raw C call stays one import away.

Source files use the `.lem` extension; the compiler, build system and package manager are all the `lem` command.

**Design rules, in priority order**

1. **If C already does it well, Lemon does it the C way.** Pointers, structs, `->`, casts, `goto`, unions, bitfields and `typedef` are unchanged; fixed-width integers get short names (`u64`, `i32`).
2. **Nothing hidden.** No hidden allocation, no hidden control flow, no destructors, no garbage collector. If memory is allocated, an allocator appears in the call or its signature.
3. **Every new feature removes boilerplate or prevents a common bug.** Features that only add expressiveness are left out.
4. **Zero-cost by default.** Generics are monomorphized, methods are plain functions, safety checks exist in debug builds and vanish in release.
5. **C interop in both directions.** Lemon imports any C header, and C can call Lemon through a generated header.

**One-line summary for C programmers:** if you can read C, you can read Lemon today; the new things are `T[]`, `Result<T>`, `?`, `defer`, `<T>`, `x.method()`, `f(name = value)`, `u64`-style integer names and `import`.

## 2. Compiler architecture and backends

The `lem` compiler lowers every program to its own intermediate representation, **LIR**, and hands LIR to a backend. Two backends are planned: a C backend first, an LLVM backend second. The language's meaning is defined on LIR, so a program behaves identically on both.

### Pipeline

1. **Preprocess** each `.lem` file: file-scoped `#define`, `#if`, `#debug`, `#test`, with build constants supplied by `lem`.
2. **Parse** and collect `import`s. Imports inside `#if` branches are resolved after preprocessing, so platform-specific imports work.
3. **Import C headers** through libclang: functions, types, enums and object-like macros become typed declarations. Function-like macros (`S_ISDIR`) are recorded as *macro calls*, checked for argument count only.
4. **Resolve** names, overloads, methods, casts and constraints across modules.
5. **Monomorphize** generics for every concrete type used.
6. **Lower to LIR.** `defer`, `?`, slices, methods, named arguments, `from`-casts and tagged unions become plain structs and explicit control flow; bounds and overflow checks become explicit LIR operations, present or absent according to the build mode.
7. **Backend:** C or LLVM, below.
8. **Link** with the system linker, together with the project's `.c` files and libraries.

### The two backends

|  | C backend (first release) | LLVM backend (later) |
| --- | --- | --- |
| Output | readable C11 with `#line` directives, compiled by gcc or clang | LLVM IR, compiled to object files directly |
| C ABI (struct passing, varargs) | handled by the C compiler | reuses clang's ABI lowering through libclang, so calls into C match clang exactly |
| Function-like C macros | emitted as written; the C compiler expands them | each macro used gets a tiny generated C shim function, compiled by clang and inlined with LTO |
| Debug info | correct Lemon file and line; variables and types appear under their C names (`Stack_u32`) | full DWARF with Lemon types: slices, generics and tagged unions display properly |
| Targets | anything with a C compiler | every LLVM target |
| Best for | bootstrapping, unusual targets, reading the output (`lem emit-c`) | the default once stable; best debugging |

Choose with `lem build --backend=c` or `--backend=llvm`, or `backend = "llvm"` under `[build]` in `lem.toml`. Until the LLVM backend is ready, the C backend is the default.

**C's undefined behaviour does not leak through the C backend.** It emits overflow-defined arithmetic (`__builtin_add_overflow` and friends, plus `-fwrapv`) and compiles with `-fno-strict-aliasing`, so the emitted C means exactly what the LIR means.

**One mangling scheme for both.** `Stack<u32>.push` becomes `Stack_u32__push`; `ds.stack.make` becomes `ds_stack__make`. Functions that are not generic, not methods and not overloaded keep their plain name, so C can call them directly and objects from either backend link together.

### Build modes

| Mode | Command | Bounds checks | Signed overflow | `#debug` code | `#test` code | `assert` |
| --- | --- | --- | --- | --- | --- | --- |
| debug (default) | `lem build` | trap | trap | compiled | stripped | active |
| release | `lem build --release` | none | wraps | stripped | stripped | stripped |
| test | `lem test` | trap | trap | compiled | compiled | active |

A project can keep checks in release with `checks = true` in `lem.toml`.

## 3. Modules and imports

One `.lem` file is one module, its name is its path under `src/` with dots, and there are no header files.

```c
// src/ds/stack.lem
module ds.stack;          // must match the file path; compile error otherwise
```

**Three kinds of import**

| Form | Meaning | Example |
| --- | --- | --- |
| `import <name.h>` | a C header; declarations come from the real header | `import <sys/wait.h>` |
| `import <name>` | a Lemon standard-library module (POSIX wrappers mirror the C header path without `.h`) | `import <sys/wait>` |
| `import a.b.c` | a module from this project or a dependency | `import ds.stack;` |

Aliases: `import ds.stack as st;`.

**Using imported names.** Names from an imported module can be used unqualified (`Stack`) or qualified by the last path component (`stack.Stack`). If two imports export the same name and the call cannot be resolved by overloading, the compiler requires qualification.

**Visibility follows C.** Every top-level declaration is exported unless it is `static`; `static` means module-private, exactly as it means file-private in C. Struct fields are always visible, as in C.

```c
module geometry;

static f64 square(f64 x) { return x * x; }   // private

f64 hypot2(f64 a, f64 b) {                   // exported
    return square(a) + square(b);
}
```

**Constants replace global macros.** A module-level `const` initialised by a constant expression is a compile-time constant, usable in array sizes:

```c
module config;
const int MAX_VAL = 100;
const usize BUFFER_SIZE = 4 * 1024;
```

```c
import config;
int table[config.MAX_VAL];
```

**Order does not matter.** Within and across modules, declarations can be used before they appear; there are no forward declarations and no prototypes. Import cycles between modules are allowed for declarations, and a cycle in `const` initialisers is a compile error.

**Platform-specific imports** work because preprocessing runs first:

```c
#if LEM_OS_LINUX
import <sys/epoll.h>
#elif LEM_OS_MACOS
import <sys/event.h>
#endif
```

## 4. Types, variables and literals

All C types keep their C meaning. Lemon adds short names for fixed-width numbers and a few built-ins, so no import is needed for the basics.

**Built-in without any import**

| Type | Meaning |
| --- | --- |
| `i8`, `i16`, `i32`, `i64` | signed integers of exactly that width (`int8_t` … `int64_t`) |
| `u8`, `u16`, `u32`, `u64` | unsigned integers of exactly that width (`uint8_t` … `uint64_t`) |
| `isize`, `usize` | pointer-sized integers (`ssize_t`, `size_t`) |
| `f32`, `f64` | `float`, `double` |
| `char`, `bool`, `true`, `false` | unchanged from C (C23 for `bool`) |
| `null` | the null pointer (`NULL` also works) |
| `byte` | alias for `u8`, used for raw memory |
| `T[]` | slice: pointer plus length (section 5) |
| `str` | alias for `const char[]`, a string view |
| `any` | a type-tagged pointer to a value, used by variadics |
| `Result<T>`, `Option<T>`, `Error` | error handling (sections 8 and 11) |

The short names are the same types as the C ones, not new types: `u64` and `uint64_t` are interchangeable, so C headers using `<stdint.h>` names need no conversion. `int`, `long`, `short`, `float` and `double` also remain, mainly for calling C. Lemon code and the standard library use the short names.

**`auto`** infers a variable's type from its initialiser at compile time, as in C23. It is allowed only for local variables with an initialiser; function parameters, return types and struct fields are always written out.

```c
auto count = 10;              // int
auto ratio = 0.5;             // f64
auto s = Stack<u32>.make();   // Stack<u32>
```

**Initialisation.** Local variables are zero-initialised unless you opt out with `uninit`. This removes the most common source of C heisenbugs; the compiler drops the zeroing when it can see the variable is written before it is read.

```c
int total;                    // 0
char buf[65536] = uninit;     // left uninitialised, for hot paths
```

Structs use C's designated and positional initialisers and compound literals, unchanged:

```c
Point p = { .x = 1, .y = 2 };
Point q = { 3, 4 };
draw((Point){ .x = 5, .y = 6 });
```

**Struct names are types.** `struct Point { ... };` declares the type `Point`; writing `struct Point` still works, and no `typedef` is needed for that.

### `typedef` and `using`

`typedef` works exactly as in C. Lemon adds the C++ alias form `using Name = Type;`, which reads left to right and can take generic parameters. Both create an alias: the new name is the same type as the old one, not a distinct type.

```c
typedef unsigned long ulong;               // C, unchanged
typedef void (*Handler)(int sig);          // function pointers, unchanged
typedef Stack<u32> IdStack;                // works with instantiated generics

using Bytes = byte[];                      // C++-style alias
using Handler2 = void (*)(int sig);        // same type as Handler
using Grid<T> = Vec<Vec<T>>;               // generic alias
using Lookup<V> = Map<str, V>;             // partially applied generic

Grid<f64> heights;                         // Vec<Vec<f64>>
Lookup<u32> ages;                          // Map<str, u32>
```

A generic alias may carry constraints like any generic declaration: `using Sorted<Comparable T> = Vec<T>;`.

### Casts and `from`

Casts between built-in types, pointers and enums mean what they mean in C. A cast **to a type that has associated `from` functions** calls the `from` overload that matches the operand's type:

```c
str name = (str)argv[1];        // str.from(const char*): wraps with strlen, no copy
str view = (str)my_string;      // str.from(const String*)
Version v = (Version)"1.4";     // Version.from(str), if Version defines one
```

Rules:

- `(T)x` is rewritten to `T.from(x)`, and normal overload resolution picks which `from` (section 6).
- Only `from` overloads that return `T` itself are usable as casts. An overload returning `Result<T>` (for example `String.from`, which allocates) must be called by name, so a cast never fails and never allocates behind your back.
- Casts are always explicit. Lemon never calls `from` for an implicit conversion.

**Integer behaviour.** Unsigned arithmetic wraps, as in C. Signed overflow traps in debug and test builds and wraps (two's complement) in release; it is never undefined behaviour.

**Implicit conversions** follow C. The compiler warns on implicit narrowing (`i64` to `i32`, `f64` to `f32`), which a cast silences.

**Literals.** C literals, plus `0b1010` binary literals and `_` digit separators (`1_000_000`). Octal `0755` keeps its C meaning; for file permissions, the readable `RWXR_XR_X` constants are also available (section 16).

## 5. Pointers, arrays and slices

Pointers and arrays are exactly C's. The one addition is the slice: `T[]` is a pointer plus a length, nothing more.

**Layout (guaranteed)**

```c
// i32[] is laid out exactly as:
struct { i32* ptr; usize len; }
```

**Making and using slices**

```c
i32 a[10];
i32[] s = a;              // array -> slice (implicit)
i32[] t = s[2:5];         // elements 2, 3, 4 (half-open)
i32[] head = s[:3];       // first three
i32[] rest = s[3:];       // from index 3 to the end

i32* p = malloc(n * sizeof(i32));
i32[] w = p[0:n];         // pointer -> slice; you vouch for n

w[i] = 1;                 // bounds-checked in debug/test, raw in release
w.ptr[i] = 1;             // never checked: the C escape hatch
usize count = w.len;      // plain field

for (i32 x : w) { ... }         // for-each by value
for (i32* x : w) { *x += 1; }   // for-each by pointer, to modify
```

**Rules**

- **Mutability:** `const T[]` is a read-only view; `T[]` is writable. A `T[]` converts implicitly to `const T[]`, never the reverse.
- **Ownership:** a slice never owns memory and never frees anything. Free what you allocated (`free(w.ptr)` or `alloc->free(w)`). A slice can dangle, exactly like a pointer.
- **Bytes:** any `T[]` converts implicitly to `byte[]`, and any `const T[]` to `const byte[]`, with `len` scaled by `sizeof(T)`. This is the slice version of `void*`, and it is what `read`, `write` and copy functions take.
- **Strings:** `str` is `const char[]`. A string literal `"abc"` is a `const char[4]` array, so it converts to `const char*` (for C) and to `str` (length 3, the terminating NUL not counted, but still present in memory). A `char*` becomes a `str` with a cast: `(str)p`.
- **Parameters:** in a Lemon parameter list, `i32[] xs` means a slice. In imported C headers, `int xs[]` keeps its C meaning (a pointer).
- **Comparison:** `==` on slices is a compile error, because it is ambiguous (same memory or same contents?). Use `mem.eq(a, b)` for contents and `a.ptr == b.ptr && a.len == b.len` for identity.
- **Passing to C:** `c_function(s.ptr, s.len)`.

The slice expression `a[lo:hi]` does not clash with the ternary: `a[c ? 1 : 2]` parses the full ternary first, and only a `:` left over after a complete expression starts a slice.

**What slices are not:** no lifetimes, no borrow checker, no separate many-item or sentinel pointer types, no `..` range syntax. A C programmer should be able to read every line above without explanation.

## 6. Functions

Functions are declared exactly as in C; Lemon adds named arguments, default values, overloading and typed variadics.

### Named and default arguments

```c
Result<int> open_file(str path, int flags = O_RDONLY, mode_t mode = RW_R__R__);

int a = open_file("a.txt")?;                                     // all defaults
int b = open_file("b.txt", O_WRONLY | O_CREAT)?;                 // positional
int c = open_file("c.txt", flags = O_WRONLY | O_CREAT, mode = RW_______)?;   // named
```

- Inside a call's argument list, `name = value` always means a named argument. It is never an assignment, and naming a parameter that does not exist is a compile error.
- Positional arguments come first, named ones after; each parameter is given at most once.
- Parameters with defaults must come after parameters without them.
- **A default may refer to earlier parameters**, which lets wrappers pick the right default from context: `int flags = (fd < 0) ? MAP_PRIVATE | MAP_ANONYMOUS : MAP_PRIVATE`.
- Default expressions are evaluated at the call site, on every call, left to right.
- Named arguments work only for Lemon functions. Parameter names in C headers are optional and unreliable, so C functions are called positionally.
- Parameter names are part of a function's public API: renaming one breaks callers that use the name.

### Overloading

Several functions may share a name if their parameter lists differ. Resolution is deliberately strict:

1. Collect every visible function with that name, from Lemon modules and C headers.
2. Keep those whose parameters accept the arguments (count, names, types). Integer literals fit any integer type that can hold them; floating literals fit any floating type.
3. An **exact type match** beats a match that needs an implicit C conversion. If more than one candidate remains at the best level, the call is ambiguous: a compile error naming every candidate.
4. **Lemon wins a tie with C.** If a Lemon function and a C function remain with identical parameter types, the Lemon one is chosen. The C one is always reachable explicitly through the `c.` prefix, e.g. `c.write(fd, p, n)`.

The same rules choose between `from` overloads when you write a cast (section 4).

```c
import <unistd.h>     // C: ssize_t write(int, const void*, size_t)
import <unistd>       // Lemon: Result<usize> write(int fd, const byte[] data, ...)

write(1, "hi\n")?;             // only the Lemon overload fits: slice argument
write(1, buf, 3);              // only the C overload fits: three arguments
c.write(1, buf, 3);            // always the raw C function
```

Overloaded functions get mangled names in the generated code. A Lemon function meant for C callers is marked `export` to force its plain name; two `export` functions with the same name are a compile error.

### Variadic functions

A Lemon variadic parameter is written `...T name` and arrives as a slice `T[]`:

```c
i64 sum(...i64 values) {
    i64 total = 0;
    for (i64 v : values) total += v;
    return total;
}

sum(1, 2, 3);        // values.len == 3
sum();               // values.len == 0
```

For mixed types, use `...any`. Each `any` holds a pointer to the argument and its type tag:

```c
void describe(...any args) {
    for (usize i = 0; i < args.len; i++) {
        if (args[i].is<i32>())        printf("i32 %d\n", args[i].as<i32>());
        else if (args[i].is<str>())   printf("str %.*s\n", (int)args[i].as<str>().len, args[i].as<str>().ptr);
        else                          printf("<%s>\n", args[i].type_name());
    }
}
```

- The caller packs the arguments into a temporary array on its own stack: no allocation, no `va_list`.
- `as<T>()` traps in debug if the tag does not match, and is unchecked in release.
- The variadic parameter must be last and cannot have a default. A slice of the right element type can be passed straight to it, which is how variadics are forwarded: `print_to(stderr, fmt, args)`.
- C variadic functions (`printf`, `open`, `ioctl`) are still called normally. Lemon cannot define new C-style `...` functions; it can only declare and call them.

### `main`

`main` may return `int` (as in C) or `Result<void>`. If it returns an error, the runtime prints `error: <msg>` to stderr and exits with the error's code (or 1 if the code is 0).

## 7. Structs and methods

Structs are C structs. A method is an ordinary function whose first parameter is named `self`; the dot-call syntax is sugar for passing the receiver.

```c
struct Counter {
    int value;
    int step;
};

void Counter.tick(Counter* self) {          // pointer receiver: can modify
    self->value += self->step;
}

int Counter.get(const Counter* self) {      // const pointer receiver: read-only
    return self->value;
}

bool Counter.is_zero(Counter self) {        // value receiver: works on a copy
    return self.value == 0;
}

Counter Counter.make(int step = 1) {        // no self: an associated function
    return (Counter){ .value = 0, .step = step };
}
```

**Calling**

```c
Counter c = Counter.make(step = 2);
c.tick();               // Counter.tick(&c)   - address taken automatically
int v = c.get();        // Counter.get(&c)

Counter* p = &c;
p->tick();              // Counter.tick(p)    - pointers use ->, as in C
bool z = p->is_zero();  // Counter.is_zero(*p)

Counter.tick(&c);       // the plain function call always works too
```

**Rules**

- The receiver form decides what is passed: `T* self` gets the address, `const T* self` gets a read-only address, `T self` gets a copy.
- Calling a `T*` method on an rvalue (`Counter.make().tick()`) is a compile error, because there is no object to point at.
- A function without a `self` first parameter is an associated function, called as `Type.name(...)`. By convention, `make` returns a new value and `free` releases what a value owns.
- There is no virtual dispatch, no inheritance and no hidden `this`. For runtime polymorphism, use a struct of function pointers, as in C.

**Methods on types you did not write.** Methods may be added to primitives, C types and types from other modules:

```c
bool int.is_even(int self) { return self % 2 == 0; }

Result<void> FILE.close_checked(FILE* self) { ... }
```

These extension methods are visible only in modules that import the module defining them. If two imported modules define the same method on the same type, an unqualified call is a compile error; qualify it as `module.Type.method(x)`. A method on a primitive can never replace a built-in operator (section 10).

## 8. Enums, tagged unions and `switch`

An `enum` without payloads is a C enum. An `enum` whose cases carry data is a tagged union: a tag plus a C union, with the compiler keeping them consistent.

```c
enum Color { RED, GREEN, BLUE };            // plain C enum, unchanged

enum Shape {
    Circle(f64 radius),
    Rect(f64 w, f64 h),
    Empty,
};

Shape a = Circle(2.0);
Shape b = Rect(w = 3.0, h = 4.0);   // named payload fields
Shape c = Empty;
```

**Matching with `switch`**

```c
f64 area(Shape s) {
    switch (s) {
        case Circle(r):   return 3.14159 * r * r;
        case Rect(w, h):  return w * h;
        case Empty:       return 0;
    }
}
```

- `switch` keeps C semantics: `break` ends a case and cases without one fall through.
- A case that binds payload names (`Circle(r)`) cannot be fallen into from the case above; that is a compile error, because its names would be unset.
- Bound names are copies scoped to their case. `case Circle(*r)` binds a pointer instead, to modify the payload in place.
- A `switch` over an enum with no `default` must list every case; a missing case is a compile error, so adding a case shows you every place to update.
- Payload fields can also be read directly when you already know the case: `s.Circle.radius` (checked in debug, unchecked in release).
- `s.tag` exposes the tag; `s.is(Circle)` tests it.

**Layout** is guaranteed: `struct { int tag; union { ... } data; }`, so tagged unions can be passed to C.

### `Option<T>`

The standard optional value is an ordinary generic tagged union:

```c
enum Option<T> { Some(T value), None };
```

Methods: `is_some()`, `is_none()`, `value()` (traps on `None` in debug), `or(T fallback)`. Use it when "no value" is a normal outcome rather than an error, such as popping an empty stack or looking up a missing key. For a pointer that may be absent, plain `T*` with `null` is still fine; `Option` is for values.

## 9. Generics and constraints

Generics are compile-time templates: the compiler emits one specialised copy per concrete type used, so generic code costs exactly what hand-written code would.

### Syntax

For functions, the generic clause comes **before** the return type, so every type parameter is declared before it is used. For structs it follows the name, as in struct Stack\<T>:

```c
struct Pair<T> { T first; T second; };

struct Entry<K, V> { K key; V value; };

<T>
void swap(T* a, T* b) { T tmp = *a; *a = *b; *b = tmp; }
```

Using them:

```c
Pair<int> p = { 1, 2 };
Entry<str, int> e = { "age", 42 };
swap(&p.first, &p.second);      // T inferred as int
swap<int>(&p.first, &p.second); // or given explicitly
```

**Parsing rule.** As in C, the parser knows which names are types. A known generic type or function followed by `<` starts a type-argument list; anything else is a less-than. `a < b > c` therefore stays a comparison.

### Constraints

A constraint names what a type must support. There are two forms.

**Method constraints** list required methods. Satisfaction is structural: any type that has the methods qualifies, with no `implements` declaration.

```c
constraint Equatable<T> {
    bool T.eq(T self, T other);
}

constraint Comparable<T> {
    int T.cmp(T self, T other);     // <0, 0, >0
}

constraint Hashable<T> {
    u64 T.hash(T self);
}
```

**Union constraints** list allowed types:

```c
constraint Integer = i8 | i16 | i32 | i64 | u8 | u16 | u32 | u64
                   | isize | usize | int | long;
constraint Float   = f32 | f64;
constraint Number  = Integer | Float;
```

**Applying constraints**

```c
<Comparable T>
T maximum(T a, T b) {
    return a > b ? a : b;           // a.cmp(b) > 0; native compare for int
}

<Number T>
T sum_all(const T[] xs) {
    T total = 0;
    for (T x : xs) total += x;
    return total;
}

struct OrderedMap<Comparable & Hashable K, V> { ... };   // several constraints: &
```

- Inside a generic body, only operations the constraint guarantees are allowed. An unconstrained `<T>` allows assignment, `sizeof(T)`, taking its address and passing it on, nothing else.
- A failed constraint is reported at the call site in one line, for example: `Version does not satisfy Comparable: missing method Version.cmp(Version, Version) -> int`.
- Built-in types satisfy `Equatable`, `Comparable` and `Hashable` through built-in methods (`int.cmp`, `int.hash`, ...). `float` and `double` are `Equatable` but **not** `Comparable`, because NaN has no position in a total order; generic code over floats uses `<Float T>` and native operators.
- A method on a generic struct can carry its own, stricter constraint. It is then available only for type arguments that satisfy it:

```c
struct List<T> { ... };

<Equatable T>
bool List<T>.contains(const List<T>* self, T value) { ... }   // only for Equatable T
```

### Generic methods

Methods on a generic struct repeat the clause:

```c
<T>
usize Stack<T>.size(const Stack<T>* self) { return self->len; }
```

### Not included

No template specialisation, no variadic generics, no compile-time code execution and no values as generic parameters. `T[]` covers most needs for array sizes.

## 10. Operator overloading

Operators on user types are rewritten to method calls with fixed names. Define the method and the operator works, including inside generic code.

| Operator | Rewritten to | Notes |
| --- | --- | --- |
| `a + b`, `a - b`, `a * b`, `a / b`, `a % b` | `a.add(b)`, `a.sub(b)`, `a.mul(b)`, `a.div(b)`, `a.mod(b)` | `+=` etc. become `a = a.add(b)` |
| `-a` | `a.neg()` |  |
| `a == b`, `a != b` | `a.eq(b)`, `!a.eq(b)` | falls back to `a.cmp(b) == 0` if only `cmp` exists |
| `a < b`, `a <= b`, `a > b`, `a >= b` | `a.cmp(b) < 0`, `<= 0`, `> 0`, `>= 0` | one `cmp` gives all four |
| `a[i]` | `a.index(i)` returning `T*`, dereferenced | one method serves reads and writes |

```c
struct Version { int major; int minor; };

int Version.cmp(Version self, Version other) {
    if (self.major != other.major) return self.major - other.major;
    return self.minor - other.minor;
}

Version a = { 1, 2 }, b = { 1, 10 };
if (a < b) { ... }                   // Version.cmp(a, b) < 0
Version newest = maximum(a, b);       // works: Version is Comparable
```

**Never overloadable**

- `&&`, `||`, `,` and `?:`: their short-circuit or sequencing rules cannot be expressed as a function call.
- Unary `&` and `*`, `->` and `.`: taking an address and dereferencing always mean what they mean in C.
- `=`: assignment is always a plain memory copy. No hidden code runs on assignment, so no hidden allocation.
- Bitwise operators and shifts: these stay integer-only, which keeps bit-twiddling code unambiguous.
- Operators on built-in types: `int + int` can never be redefined.

**Inside generics**, an operator on `T` resolves to the method after monomorphization. For built-in types it compiles to the native instruction, so `maximum<int>` is a single compare.

## 11. Error handling

Failures are values: a function that can fail returns `Result<T>`, `?` passes an error to the caller, and `defer` makes sure cleanup still runs.

### `Error` and `Result<T>`

```c
struct Error {
    int code;           // errno value for OS errors, or a module's own code
    const char* msg;    // static string: never allocated, never freed
};

enum Result<T> { Ok(T value), Err(Error error) };
```

| Method | Meaning |
| --- | --- |
| `r.is_ok()`, `r.is_error()` | test the case |
| `r.value()` | the value; traps with the error's message if it is an error |
| `r.error()` | the `Error`; traps if it is a value |
| `r.or(fallback)` | the value, or `fallback` on error |
| `r.expect("context")` | the value, or print `context: <msg>` and exit with the error's code |

Constructing results:

```c
return Ok(42);                              // Result<int>
return Ok();                                // Result<void>
return Err(EINVAL, "length must be > 0");   // code + static message
return Err(Error.from_errno());             // errno -> Error, with a static message
```

**A `Result` cannot be ignored.** Calling a function that returns `Result` and discarding it is a compile error. Write `(void)f();` to discard on purpose.

Results can also be matched: `switch (r) { case Ok(v): ...; break; case Err(e): ...; }`.

### `?`: pass the error up

`expr?` evaluates `expr`; if it is `Err(e)`, the current function returns `Err(e)` immediately (after running pending `defer`s); otherwise the expression is the value.

```c
Result<int> parse_and_add(str a, str b) {
    int x = parse_int(a)?;
    int y = parse_int(b)?;
    return Ok(x + y);
}
```

- `?` is allowed only in functions that return a `Result<U>`; the error carries over whatever `U` is.
- `?` never exits the program and never frees anything by itself. Cleanup is `defer`'s job.

### `defer` and `errdefer`

`defer statement;` runs the statement when the enclosing **block** ends, by any route: falling off the end, `return`, `break`, `continue` or `?`. Several defers run in reverse order.

`errdefer statement;` runs only when the function is left with an error (an `Err` return or a failed `?`). It is for undoing partial work in constructors.

```c
Result<Buffer> load(str path) {
    int fd = open(path)?;
    defer close(fd);                    // always closes

    byte[] data = heap->array<byte>(4096)?;
    errdefer heap->free(data);          // freed only if we fail below

    usize n = read(fd, data)?;          // on error: free data, close fd, return Err
    return Ok((Buffer){ .data = data, .len = n });   // success: close fd, keep data
}
```

Rules: a deferred statement may discard a Result (there is nowhere to send it) but cannot `return` or use `?`; it sees variables as they are when it runs, not when it was declared; a return value is computed before deferred statements run; `goto` may not jump forward past a `defer` into its block.

### Panics

A panic prints a message with the source location and calls `abort()`. There is no unwinding and nothing to catch. Panics come from `value()` or `error()` on the wrong case, a failed `assert`, an out-of-bounds index or signed overflow in debug builds, and an `any` tag mismatch in debug builds. `expect` is the polite variant: it prints `context: <msg>` and calls `exit()` with the error's code instead of aborting. Use panics for bugs, and `Result` for things that can go wrong at run time.

## 12. Memory and allocators

Memory is manual, as in C. `malloc` and `free` work unchanged; Lemon adds an `Allocator` interface so that code can choose its allocation strategy, and every allocating standard-library function takes the allocator as a defaulted parameter.

### The rule: the allocator is in the signature

```c
String String.from(str s, Allocator* alloc = heap);
```

Called without it, `String.from("hi")` uses the heap (`malloc`). Called with it, the caller decides: `String.from("hi", alloc = arena.allocator())`. Allocation is never hidden, because the parameter is always visible in the declaration, but it costs nothing to type in the common case.

### `Allocator`

An allocator is a plain struct of function pointers plus a context pointer, so it works across the C boundary:

```c
struct Allocator {
    void* ctx;
    void* (*alloc_fn)(void* ctx, usize size, usize align);
    void* (*resize_fn)(void* ctx, void* ptr, usize old_size, usize new_size, usize align);
    void  (*free_fn)(void* ctx, void* ptr, usize size);
};
```

Typed helper methods sit on top, and all of them return `Result`, so running out of memory is an ordinary error:

| Method | Returns |
| --- | --- |
| `alloc->create<T>()` | `Result<T*>`, one zeroed `T` |
| `alloc->array<T>(n)` | `Result<T[]>`, `n` zeroed elements |
| `alloc->resize<T>(s, n)` | `Result<T[]>`, contents kept, may move |
| `alloc->destroy<T>(p)` | frees a `create`d pointer |
| `alloc->free<T>(s)` | frees an `array`ed slice |

### Allocators in `<mem>`

| Allocator | Use it for | Freeing |
| --- | --- | --- |
| `heap` | general allocation; wraps `malloc`/`realloc`/`free` | per allocation |
| `Arena` | many objects with one lifetime (a request, a parse, a frame) | all at once with `reset()` or `free()` |
| `Pool<T>` | many same-sized objects, constantly created and destroyed | per object, O(1) |
| `scratch()` | short-lived temporaries inside one function; a per-thread arena | automatically when the function returns (`defer` inside) |
| `DebugAllocator` | default `heap` in debug and test builds | reports leaks, double frees and use-after-free at exit |

```c
import <mem>

Arena arena = Arena.make(1024 * 1024)?;      // 1 MiB block, grows if needed
defer arena.free();

Allocator* a = arena.allocator();
i32[] values = a->array<i32>(100)?;           // no individual free needed
String name = String.from("lemon", alloc = a)?;
```

**Debug safety without a borrow checker.** In debug and test builds `heap` is a `DebugAllocator`: it pads allocations with guard bytes, poisons freed memory and prints every leaked allocation with the source line that made it. Release builds use plain `malloc`.

There is no garbage collector, no reference counting and no destructor: whoever allocates decides when to free, and `defer` keeps that next to the allocation.

## 13. Strings

Lemon has two string types, one owned and one borrowed, and both interoperate with `char*` in one call.

| Type | What it is | Owns memory | NUL-terminated |
| --- | --- | --- | --- |
| `str` | `const char[]`: pointer + length view | no | not guaranteed |
| `String` | growable buffer: pointer, length, capacity, allocator | yes | always |
| `char*` / `const char*` | C strings, unchanged | caller decides | yes |

```c
import <string>

str greeting = "hello";                     // literal -> str, no allocation
String s = String.from(greeting)?;          // copy onto the heap
defer s.free();

s.append(", world")?;
s.appendf(" #{}", 42)?;                      // formatted append
printf("%s\n", s.cstr());                    // always NUL-terminated: free
str view = s.view();                         // borrow as str

String path = String.format("{}/{}.log", dir, name)?;
```

**Rules**

- `String` never reallocates behind your back except in methods that return `Result` (`append`, `appendf`, `reserve`), so every allocation point can fail visibly.
- `String` keeps a NUL byte after its contents at all times, so `cstr()` is a free pointer cast, never a copy.
- `str` comes from literals, slicing (`s.view()[0:5]`) or `(str)p` (which calls `strlen`). A `str` cut from the middle of a string is not NUL-terminated; wrappers that need a C string copy it to a stack buffer (section 16).
- `str` methods do not allocate: `eq`, `cmp`, `starts_with`, `ends_with`, `find`, `split`, `trim`, `parse_int`, `parse_float`. `str` is `Comparable` and `Hashable`, so it works as a map key.
- Text is bytes. Lemon validates and iterates UTF-8 (`s.utf8_valid()`, `for (u32 cp : s.codepoints())`) but does no normalisation or locale handling.

**Formatting.** `{}` placeholders take any argument, through `...any`. `{}` prints the value in its natural form, `{x}` in hex, `{:8}` padded, `{:.3}` with precision. Types with a `T.fmt(T self, String* out)` method print themselves. `printf` remains available and unchanged.

## 14. Preprocessor, `#debug` and `#test`

The C preprocessor stays, but everything it defines is local to one file. Anything shared between files is a typed `const` exported from a module (section 3).

### What remains

| Directive | Behaviour in Lemon |
| --- | --- |
| `#define`, `#undef` | as in C, but visible only in the current file |
| `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif` | as in C |
| `#error`, `#warning`, `#pragma` | as in C |
| `#include` | compile error with a hint: use `import` |
| `#debug` … `#end_debug` | code that exists only in debug and test builds |
| `#test` … `#end_test` | code that exists only under `lem test`; every function inside is a test |

**Build constants** are predefined in every file and can be tested with `#if`:

| Constant | Value |
| --- | --- |
| `LEM_DEBUG`, `LEM_RELEASE`, `LEM_TEST` | 1 in that build mode, else 0 (`LEM_DEBUG` is also 1 under `lem test`) |
| `LEM_OS_LINUX`, `LEM_OS_MACOS`, `LEM_OS_FREEBSD` | 1 on that OS |
| `LEM_ARCH_X86_64`, `LEM_ARCH_ARM64` | 1 on that architecture |
| `LEM_BACKEND_C`, `LEM_BACKEND_LLVM` | 1 for the backend in use |
| `LEM_VERSION` | compiler version, e.g. `300` for 0.3.0 |

Project-wide flags come from `lem.toml` (`[defines] USE_EPOLL = 1`) and are visible in every file of the project, which is how global configuration is done without global macros.

### `#debug` … `#end_debug`

Code inside a `#debug` block exists only in debug and test builds. It can appear at top level or inside a function body.

```c
#debug
void log(str fmt, ...any args) {
    fprintf(stderr, "[debug] ");
    print_to(stderr, fmt, args);
}

u64 calls_made = 0;
#end_debug

i32 work(i32 x) {
    log("work({})", x);     // removed entirely in release
#debug
    calls_made++;
#end_debug
    return x * 2;
}
```

Rules:

1. In release builds, every call to a function declared in a `#debug` block is removed, **including the evaluation of its arguments**. `log("{}", expensive())` costs nothing in release. The arguments are still type-checked, so release builds cannot break silently.
2. A function declared in a `#debug` block must return `void`; otherwise there would be no value to use in release.
3. Variables and types declared in a `#debug` block may be used only inside other `#debug` code or as arguments to `#debug` functions. Using them anywhere else is a compile error.
4. `assert(cond)` behaves exactly like a `#debug` function.

### `#test` … `#end_test`

Every function inside a `#test` block is a test. Test blocks are compiled only by `lem test` and can see everything in their module, including `static` (module-private) functions and variables.

```c
module ds.stack;

static usize grow_capacity(usize cap) { return cap == 0 ? 8 : cap * 2; }

#test
void capacity_doubles() {
    assert(grow_capacity(0) == 8);         // private function: visible here
    assert(grow_capacity(8) == 16);
}

Result<void> push_then_pop() {
    Stack<i32> s = Stack<i32>.make();
    defer s.free();
    s.push(7)?;
    assert(s.pop().value() == 7);
    return Ok();
}
#end_test
```

Rules:

1. A test function takes no parameters and returns `void` or `Result<void>`. A returned `Err` fails the test and prints its message.
2. The test's name is its function name, prefixed by its module: `ds.stack.capacity_doubles`.
3. Types, constants and variables may also be declared inside a `#test` block as fixtures; they are not tests.
4. Helper functions that take parameters cannot live in a `#test` block. Put them in a file named `*.test.lem` (for example `src/ds/stack.test.lem`), which is compiled only under `lem test` and declares `module ds.stack;` so it shares the module's private scope.
5. Each test runs in its own forked process, so a panic or a segfault fails that test without stopping the run.
6. `lem test` runs every test in the project; `lem test stack` runs those whose name contains `stack`.

### Macros from C headers

Object-like macros from imported C headers (`EINTR`, `O_CREAT`, `PATH_MAX`) become constants. Function-like macros (`S_ISDIR(m)`, `WEXITSTATUS(s)`) can be called as written; each backend makes them work as described in section 2.

## 15. C interoperability

Lemon calls any C library and any C code can call Lemon, with no binding generator in either direction.

### Calling C from Lemon

```c
import <zlib.h>          // declarations straight from the real header

int main() {
    const char* v = zlibVersion();
    printf("zlib %s\n", v);
    return 0;
}
```

- Link flags come from `lem.toml` (`[link] libs = ["z"]`, or `pkg-config = ["zlib"]`).
- All C types, structs, enums, unions, function pointers and macros from the header are usable. C struct tags keep their own namespace, so a C struct that shares a name with a function (`struct stat` and `stat`) is written with the `struct` keyword.
- A C function returning `int` stays an `int`; Lemon never guesses an error convention. Wrapping in `Result` is what the POSIX layer (section 16) is for, and what you do for other libraries.
- C functions are called positionally (no named arguments) and keep C's implicit conversions.
- Slices pass as two arguments: `crc32(0, data.ptr, data.len)`.

### Calling Lemon from C

`lem build` writes a C header for each module into `build/include/`. Plain functions keep their names; generic, method and overloaded functions are emitted under mangled names unless marked `export`.

```c
// src/mathx.lem
module mathx;

export i32 clamp_i32(i32 v, i32 lo = 0, i32 hi = 100) {
    return v < lo ? lo : v > hi ? hi : v;
}
```

```c
/* generated: build/include/mathx.h */
int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi);   /* C callers pass every argument */
```

What C sees for Lemon types:

| Lemon | C |
| --- | --- |
| `i32[]` | `struct lem_slice_i32 { int32_t* ptr; size_t len; }` |
| `Result<i32>` | `struct lem_result_i32 { int tag; union { int32_t value; Error error; }; }` |
| `enum Shape { Circle(f64), ... }` | `struct Shape { int tag; union { ... } data; }` |
| `Stack<u32>` | `struct Stack_u32 { ... }` |
| methods | functions taking the receiver as their first parameter |

### Mixing source files

Put `.c` files in `src/` next to `.lem` files. `lem build` compiles both and links them together, which allows converting a C project one file at a time: rename `foo.c` to `foo.lem`, add `module foo;`, replace its `#include`s with `import`s, and its callers keep working through the generated header.

## 16. The POSIX layer

The POSIX layer is Lemon's reason to exist: the same system calls C programmers already know, with the same names, made hard to misuse. Every wrapper follows the same ten rules, so learning one teaches all of them.

### The ten rules

1. **Same path, same name.** `<sys/mman.h>` becomes `import <sys/mman>`; `mmap` stays `mmap`. Nothing to relearn.
2. **Errors are `Result`.** Every call that can fail returns `Result<T>`. The `Error` code is the `errno` value; the message is a static, thread-safe string (no `strerror` buffer races).
3. **Pointer plus length becomes a slice.** `write(fd, buf, n)` becomes `write(fd, data)`; the number of bytes is always the slice's own length.
4. **Paths are `str`.** If the `str` is not NUL-terminated, the wrapper copies it into a `PATH_MAX` stack buffer, and an over-long path returns `ENAMETOOLONG` instead of overflowing.
5. **Out-parameters become return values.** `stat(path)` returns `Result<struct stat>`, `pipe()` returns `Result<Pipe>`.
6. **Packed status codes become tagged unions.** `waitpid` returns `Exited(code)`, `Signaled(sig)` or `Stopped(sig)` instead of an `int` to decode with macros.
7. **Common flags get defaults, everything stays overridable** through named arguments.
8. **`EINTR` is retried by default** for calls where retrying is safe (`read`, `write`, `waitpid`), controlled by `retry_eintr = true`. `close` is never retried, since a retry can close another thread's descriptor.
9. **No hidden allocation.** Wrappers use only the stack. Helpers that must allocate (`read_all`, `list_dir`) take an `Allocator*` parameter defaulting to `heap`.
10. **The C header is imported along with the wrapper module, and the raw call is always one prefix away:** `c.mmap(...)`, with C's exact signature and behaviour.

File descriptors stay plain `int`s. There are no handle objects and no destructors; close them with `defer close(fd)`.

### Permission constants

Every wrapper that takes a file mode accepts either an octal number or a readable permission constant. `<sys/stat>` defines the enum `Perm` with all 512 combinations, one name per pattern:

- Nine characters: owner, group, others, three each, always in the order `R`, `W`, `X`.
- `_` marks a missing permission, where `ls -l` would show `-`.

```c
enum Perm : mode_t {          // generated in <sys/stat>, all 512 values
    _________ = 0000,
    R________ = 0400,
    RW_______ = 0600,
    RWX______ = 0700,
    RW_R__R__ = 0644,
    RWXR_XR_X = 0755,
    RWXRWX___ = 0770,
    RWXRWXRWX = 0777,
    // ... and every other combination
};

enum PermExtra : mode_t { SETUID = 04000, SETGID = 02000, STICKY = 01000 };
```

```c
mkdir("shared", mode = RWXRWX___)?;              // same as mode = 0770
open("notes.txt", O_WRONLY | O_CREAT, mode = RW_R__R__)?;
chmod("/tmp/drop", RWXRWXRWX | STICKY)?;         // 01777
```

`Perm` values convert implicitly to `mode_t`, as C enums convert to integers, so they also work with raw C calls: `c.mkdir("x", RWXR_XR_X)`. Octal literals keep working everywhere; the constants exist so that nobody has to decode `0755` to read a program.

### C vs Lemon at a glance

| Task | C | Lemon |
| --- | --- | --- |
| Write a buffer | `if (write(fd, buf, len) < 0) { perror("write"); exit(1); }` | `write(fd, buf)?;` |
| Make a directory | `if (mkdir("out", 0755) < 0 && errno != EEXIST) ...` | `mkdir("out", exist_ok = true)?;` |
| Anonymous memory | `void* p = mmap(NULL, n, PROT_READ \| PROT_WRITE, MAP_PRIVATE \| MAP_ANONYMOUS, -1, 0); if (p == MAP_FAILED) ...` | `byte[] mem = mmap(n)?;` |
| File size | `struct stat st; if (stat(path, &st) < 0) ...; st.st_size` | `stat(path)?.st_size` |
| Child exit code | `waitpid(pid, &status, 0); if (WIFEXITED(status)) WEXITSTATUS(status)` | `switch (waitpid(pid)?) { case Exited(code): ... }` |
| Pipe | `int fds[2]; if (pipe(fds) < 0) ...` | `Pipe p = pipe()?;` |

### Modules

| Module | Wraps |
| --- | --- |
| `<unistd>` | `read`, `write`, `close`, `lseek`, `pipe`, `dup2`, `fork`, `execvp`, `getcwd`, `chdir`, `unlink`, `rmdir`, `access`, `sleep` |
| `<fcntl>` | `open`, `openat`, `fcntl` |
| `<sys/stat>` | `stat`, `fstat`, `lstat`, `mkdir`, `chmod`, `Perm` constants |
| `<sys/mman>` | `mmap`, `munmap`, `mprotect`, `msync`, `madvise` |
| `<sys/wait>` | `waitpid`, `wait` |
| `<dirent>` | `opendir`, `readdir`, `closedir`, plus `list_dir` |
| `<signal>` | `sigaction`, `kill`, `raise`, signal masks |
| `<sys/socket>`, `<netinet/in>`, `<arpa/inet>` | sockets, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, addresses |
| `<poll>` | `poll` |
| `<pthread>` | threads, mutexes, condition variables |
| `<time>` | `clock_gettime`, `nanosleep` |

Full wrapper implementations for `write`, `mkdir` and `mmap` are in section 20, Examples.

## 17. Standard library

The standard library is small and systems-focused: the POSIX layer plus the containers, strings and allocators that systems code needs every day. Anything else belongs in packages.

| Module | Contents |
| --- | --- |
| `<io>` | `print`, `println`, `eprintln`, `print_to`, buffered `Reader`/`Writer` over a file descriptor, `stdin`/`stdout`/`stderr` |
| `<string>` | `String`, `str` methods, `String.format`, UTF-8 helpers |
| `<mem>` | `Allocator`, `heap`, `Arena`, `Pool<T>`, `scratch()`, `DebugAllocator`, `mem.copy`, `mem.set`, `mem.eq` |
| `<collections>` | `Vec<T>` (growable array), `Map<K, V>` (hash map), `Set<T>`, `Deque<T>`, `Heap<T>`; all allocator-aware |
| `<sort>` | `sort<Comparable T>(T[])`, `sort_by(T[], cmp_fn)`, `binary_search` |
| `<math>` | generic `min`, `max`, `clamp`, `abs` over `Number`; checked and wrapping arithmetic (`checked_add`, `wrapping_mul`) |
| `<fs>` | higher-level helpers built on the POSIX layer: `read_file`, `write_file`, `copy_file`, `walk`, `exists`, path joining |
| `<process>` | `spawn` (fork + exec + pipes in one call), `run` (spawn and wait), `exit`, environment variables, arguments |
| `<thread>` | `Thread`, `Mutex`, `CondVar`, `Once`, atomics; thin wrappers over `<pthread>` |
| `<time>` | monotonic and wall clocks, durations, `sleep`, formatting |
| `<test>` | `assert_eq`, `assert_err`, test fixtures and the runner hooks used by `lem test` |
| POSIX modules | see section 16 |

The C standard library is always available through `import <stdio.h>` and friends, unchanged.

**Deliberately not in the standard library:** JSON, HTTP, regular expressions, GUI, cryptography, async I/O frameworks. These change too fast or are too opinionated for a standard library and are better served as packages.

## 18. Build system and package manager

One tool, `lem`, builds, runs, tests, documents and fetches dependencies; a project needs no Makefile.

| Command | Does |
| --- | --- |
| `lem init name` | creates a project: `lem.toml`, `src/main.lem`, `.gitignore` |
| `lem init name --lib` | same, for a library (`src/lib.lem`, no `main`) |
| `lem build` | debug build into `build/debug/` |
| `lem build --release` | optimised build into `build/release/` |
| `lem build --backend=llvm` | build with the LLVM backend instead of C |
| `lem run [-- args]` | build and run |
| `lem test [filter]` | build in test mode and run matching tests |
| `lem doc` | generate HTML docs from `///` comments into `build/doc/` |
| `lem add name --git url [--tag v1.2.0]` | add a dependency |
| `lem fmt` | format source files |
| `lem emit-c` | write the generated C to `build/c/` for inspection |

### Project layout

```
myproj/
  lem.toml
  lem.lock            # exact dependency commits, generated
  src/
    main.lem          # module main
    ds/stack.lem      # module ds.stack
    ds/stack.test.lem # test helpers for ds.stack
    legacy/parser.c   # plain C, compiled and linked too
  build/              # all output, generated
```

### `lem.toml`

```toml
[package]
name = "myproj"
version = "0.1.0"
kind = "bin"             # or "lib" / "shared-lib"

[dependencies]
colors = { git = "https://github.com/someone/colors", tag = "v1.2.0" }

[link]
libs = ["m", "pthread"]
pkg-config = ["zlib"]

[defines]
USE_EPOLL = 1

[build]
backend = "c"           # or "llvm"
cc = "clang"            # C backend only; default: $CC, then cc
c-flags = ["-march=native"]
checks = false          # true keeps bounds and overflow checks in release
```

**Dependencies** are Git repositories pinned to a tag or commit and recorded in `lem.lock`. A dependency is just another Lemon project; its modules are imported by their own names (`import colors;`). In a library, `src/lib.lem` is the module named after the package and `src/x/y.lem` becomes `colors.x.y`. A central registry can come later without changing this model.

**Documentation** comes from `///` comments placed directly above a declaration. `lem doc` documents every exported declaration and skips `static` ones; ordinary `//` comments never appear.

## 19. Safety and undefined behaviour

Lemon keeps C's power to write unsafe code but removes the cheap-to-remove undefined behaviour and makes the rest trap in debug builds. The rules below hold on both backends.

| Situation | C | Lemon debug / test | Lemon release |
| --- | --- | --- | --- |
| Slice index out of bounds | UB | panic with index, length, location | unchecked |
| Raw pointer or array out of bounds | UB | UB | UB |
| Signed integer overflow | UB | panic | wraps (defined) |
| Unsigned overflow | wraps | wraps | wraps |
| Reading an uninitialised local | UB | impossible: zero-initialised (unless `uninit`) | same |
| Ignoring an error return | allowed | compile error for `Result` | same |
| Wrong case of a tagged union | n/a | panic | unchecked |
| `any.as<T>()` with the wrong `T` | n/a | panic | unchecked |
| Null pointer dereference | UB | UB (usually a segfault) | UB |
| Use after free, double free | UB | reported by `DebugAllocator` | UB |
| Memory leak | silent | reported at exit by `DebugAllocator` | silent |
| Division by zero | UB | panic | UB |
| Shift by >= bit width | UB | panic | masked to bit width (defined) |
| Data race | UB | UB | UB |

The remaining UB (raw pointers, null, data races, use after free in release) is the cost of staying C. Lemon does not attempt to remove it with a borrow checker; it gives you slices, `Result`, `defer` and a debug allocator so that you rarely need to go near it.

**Release with checks.** `checks = true` in `lem.toml` keeps every debug-mode check in release builds, for programs where a crash is better than corruption.

## 20. Examples

Every example below is complete Lemon code written against this specification.

### 20.1 Hello, world

The C way still works, unchanged apart from `import`:

```c
import <stdio.h>

int main() {
    printf("Hello, world!\n");
    return 0;
}
```

The Lemon way, with a `from`-cast and formatting through `...any`:

```c
import <io>

int main(int argc, char** argv) {
    str name = argc > 1 ? (str)argv[1] : "world";      // (str) calls str.from(const char*)
    println("Hello, {}! You passed {} argument(s).", name, argc - 1);
    return 0;
}
```

### 20.2 Fibonacci

A plain version, a version that reports overflow as an error, and tests in the same file.

```c
import <io>
import <math>

/// Returns the n-th Fibonacci number. Wraps silently past fib(93).
u64 fib(u32 n) {
    if (n == 0) return 0;
    u64 a = 0, b = 1;                 // fib(0), fib(1)
    for (u32 i = 1; i < n; i++) {
        u64 next = a + b;
        a = b;
        b = next;
    }
    return b;                         // fib(n)
}

/// Same, but returns an error instead of a wrong answer.
Result<u64> fib_checked(u32 n) {
    if (n == 0) return Ok(0);
    u64 a = 0, b = 1;
    for (u32 i = 1; i < n; i++) {
        u64 next = checked_add(a, b)?;    // Err(ERANGE, "integer overflow")
        a = b;
        b = next;
    }
    return Ok(b);
}

/// Recursive form, for comparison. Exponential time: fine for small n only.
u64 fib_slow(u32 n) {
    return n < 2 ? n : fib_slow(n - 1) + fib_slow(n - 2);
}

int main() {
    for (u32 i = 0; i <= 10; i++)
        println("fib({}) = {}", i, fib(i));

    println("fib(93) = {}", fib_checked(93).expect("fib(93)"));

    Result<u64> r = fib_checked(94);
    if (r.is_error())
        println("fib(94): {}", r.error().msg);   // "integer overflow"
    return 0;
}

#test
void small_values() {
    assert(fib(0) == 0);
    assert(fib(1) == 1);
    assert(fib(10) == 55);
}

void iterative_matches_recursive() {
    for (u32 n = 0; n < 25; n++) assert(fib(n) == fib_slow(n));
}

void overflow_is_an_error() {
    assert(fib_checked(93).is_ok());
    assert(fib_checked(94).is_error());
}
#end_test
```

### 20.3 Generic stack

A growable stack backed by a slice, allocator-aware, with tests that use a private helper.

```c
// src/ds/stack.lem
module ds.stack;

import <mem>

/// A growable last-in, first-out stack.
struct Stack<T> {
    T[] buf;            // buf.len is the capacity
    usize len;          // items in use
    Allocator* alloc;
};

static usize grow_capacity(usize cap) { return cap == 0 ? 8 : cap * 2; }

/// Creates an empty stack. Nothing is allocated until the first push.
<T>
Stack<T> Stack<T>.make(Allocator* alloc = heap) {
    return (Stack<T>){ .alloc = alloc };      // buf and len are zero
}

/// Pushes a value, growing the buffer if needed.
<T>
Result<void> Stack<T>.push(Stack<T>* self, T value) {
    if (self->len == self->buf.len) {
        // resize of an empty slice behaves like a fresh allocation
        self->buf = self->alloc->resize(self->buf, grow_capacity(self->buf.len))?;
    }
    self->buf[self->len++] = value;
    return Ok();
}

/// Removes and returns the top value, or None if the stack is empty.
<T>
Option<T> Stack<T>.pop(Stack<T>* self) {
    if (self->len == 0) return None;
    return Some(self->buf[--self->len]);
}

/// Pointer to the top value, or null. Valid until the next push.
<T>
T* Stack<T>.peek(Stack<T>* self) {
    return self->len == 0 ? null : &self->buf[self->len - 1];
}

<T>
usize Stack<T>.size(const Stack<T>* self) { return self->len; }

<T>
bool Stack<T>.is_empty(const Stack<T>* self) { return self->len == 0; }

/// The items currently on the stack, bottom first. Valid until the next push.
<T>
const T[] Stack<T>.items(const Stack<T>* self) { return self->buf[0:self->len]; }

/// Releases the buffer. The stack can be reused afterwards.
<T>
void Stack<T>.free(Stack<T>* self) {
    self->alloc->free(self->buf);
    *self = (Stack<T>){ .alloc = self->alloc };
}

#test
void lifo_order() {
    Stack<i32> s = Stack<i32>.make();
    defer s.free();
    for (i32 i = 0; i < 100; i++) s.push(i).expect("push");
    assert(s.size() == 100);
    for (i32 i = 99; i >= 0; i--) assert(s.pop().value() == i);
    assert(s.pop().is_none());
}

void capacity_doubles() {
    assert(grow_capacity(0) == 8);      // static helper, visible to tests
    assert(grow_capacity(8) == 16);
}

void peek_does_not_remove() {
    Stack<str> s = Stack<str>.make();
    defer s.free();
    s.push("a").expect("push");
    assert(s.peek()->eq("a"));
    assert(s.size() == 1);
}
#end_test
```

Using it, with the default heap and with an arena, plus a `using` alias:

```c
// src/main.lem
import ds.stack;
import <mem>
import <io>

using WordStack = Stack<str>;

Result<void> main() {
    WordStack words = WordStack.make();
    defer words.free();

    words.push("systems")?;
    words.push("loves")?;
    words.push("Lemon")?;
    while (!words.is_empty())
        println("{}", words.pop().value());     // Lemon, loves, systems

    Arena arena = Arena.make(64 * 1024)?;
    defer arena.free();                        // one free for everything below
    Stack<u64> nums = Stack<u64>.make(alloc = arena.allocator());
    for (u64 i = 0; i < 1000; i++) nums.push(i * i)?;
    println("top = {}", *nums.peek());
    return Ok();
}
```

### 20.4 Generic singly linked list

Nodes come from the list's allocator; `find` and `remove` exist only for element types that are `Equatable`. Traversal is a plain C `for` loop.

```c
// src/ds/list.lem
module ds.list;

import <mem>

struct Node<T> {
    T value;
    Node<T>* next;
};

struct List<T> {
    Node<T>* head;
    Node<T>* tail;
    usize len;
    Allocator* alloc;
};

<T>
List<T> List<T>.make(Allocator* alloc = heap) {
    return (List<T>){ .alloc = alloc };
}

<T>
Result<void> List<T>.push_front(List<T>* self, T value) {
    Node<T>* n = self->alloc->create<Node<T>>()?;   // zeroed: n->next == null
    n->value = value;
    n->next = self->head;
    self->head = n;
    if (self->tail == null) self->tail = n;
    self->len++;
    return Ok();
}

<T>
Result<void> List<T>.push_back(List<T>* self, T value) {
    Node<T>* n = self->alloc->create<Node<T>>()?;
    n->value = value;
    if (self->tail) self->tail->next = n;
    else            self->head = n;
    self->tail = n;
    self->len++;
    return Ok();
}

<T>
Option<T> List<T>.pop_front(List<T>* self) {
    Node<T>* n = self->head;
    if (n == null) return None;
    defer self->alloc->destroy(n);       // runs after the return value is computed
    self->head = n->next;
    if (self->head == null) self->tail = null;
    self->len--;
    return Some(n->value);
}

/// First node holding `value`, or null.
<Equatable T>
Node<T>* List<T>.find(List<T>* self, T value) {
    for (Node<T>* n = self->head; n != null; n = n->next)
        if (n->value == value) return n;             // T.eq
    return null;
}

/// Removes the first node holding `value`. Returns whether one was found.
<Equatable T>
bool List<T>.remove(List<T>* self, T value) {
    Node<T>* prev = null;
    for (Node<T>* n = self->head; n != null; prev = n, n = n->next) {
        if (n->value != value) continue;
        if (prev) prev->next = n->next;
        else      self->head = n->next;
        if (self->tail == n) self->tail = prev;
        self->alloc->destroy(n);
        self->len--;
        return true;
    }
    return false;
}

/// Reverses the list in place.
<T>
void List<T>.reverse(List<T>* self) {
    Node<T>* prev = null;
    Node<T>* cur = self->head;
    self->tail = self->head;
    while (cur) {
        Node<T>* next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    self->head = prev;
}

/// Frees every node. The list can be reused afterwards.
<T>
void List<T>.free(List<T>* self) {
    Node<T>* n = self->head;
    while (n) {
        Node<T>* next = n->next;
        self->alloc->destroy(n);
        n = next;
    }
    *self = (List<T>){ .alloc = self->alloc };
}

#test
void push_pop_reverse_remove() {
    List<i32> l = List<i32>.make();
    defer l.free();
    for (i32 i = 1; i <= 3; i++) l.push_back(i).expect("push");   // 1 2 3
    l.reverse();                                                   // 3 2 1
    assert(l.pop_front().value() == 3);                            // 2 1
    assert(l.find(1) != null);
    assert(l.remove(2));                                           // 1
    assert(!l.remove(42));
    assert(l.len == 1 && l.head == l.tail);
}

void empty_list() {
    List<i32> l = List<i32>.make();
    assert(l.pop_front().is_none());
    l.reverse();
    assert(l.head == null && l.tail == null);
}
#end_test
```

Printing a list needs nothing new:

```c
import ds.list;
import <io>

Result<void> main() {
    List<str> l = List<str>.make();
    defer l.free();
    l.push_back("a")?;
    l.push_back("b")?;
    l.push_front("z")?;

    for (Node<str>* n = l.head; n; n = n->next)
        print("{} ", n->value);          // z a b
    println("");
    return Ok();
}
```

### 20.5 POSIX wrapper: `write`

This is how the standard library implements it. Inside the wrapper, the raw call is written `c.write` so there is no doubt which one runs.

```c
// std/unistd.lem  (imported as <unistd>)
module unistd;

import <unistd.h>
import <errno.h>

/// Writes some of `data` to `fd` and returns how many bytes were written.
/// Like write(2) it may write fewer bytes than given; see write_all.
Result<usize> write(int fd, const byte[] data, bool retry_eintr = true) {
    while (true) {
        isize n = c.write(fd, data.ptr, data.len);
        if (n >= 0) return Ok((usize)n);
        if (errno == EINTR && retry_eintr) continue;
        return Err(Error.from_errno());
    }
}

/// Writes all of `data`, looping over short writes.
Result<void> write_all(int fd, const byte[] data) {
    while (data.len > 0) {
        usize n = write(fd, data)?;
        data = data[n:];               // the slice moves; the bytes do not
    }
    return Ok();
}
```

Calling it: any slice converts to `const byte[]`, so strings, `String` views and arrays of structs all work.

```c
import <unistd>

write_all(STDOUT_FILENO, "hello\n")?;
write_all(fd, records)?;               // Record[] -> const byte[]
usize n = write(fd, buf[0:used])?;     // partial write is allowed
```

### 20.6 POSIX wrapper: `mkdir`

Adds `mkdir -p` behaviour and an `exist_ok` flag. Note the default `exist_ok = parents`, which refers to an earlier parameter, the readable default mode `RWXR_XR_X`, and the stack buffer that turns a `str` into a C string without allocating.

```c
// std/sys/stat.lem  (imported as <sys/stat>)
module sys.stat;

import <sys/stat.h>
import <errno.h>
import <limits.h>

/// Creates a directory.
///   mode     = an octal number or a Perm constant such as RWXRWX___
///   parents  = true: also create missing parent directories, like mkdir -p
///   exist_ok = true: succeed if the directory already exists
Result<void> mkdir(str path, mode_t mode = RWXR_XR_X,
                   bool parents = false, bool exist_ok = parents) {
    char buf[PATH_MAX] = uninit;
    char* cpath = path.copy_cstr(buf)?;     // copies + NUL; ENAMETOOLONG if too long

    if (parents) {
        // "a/b/c": create "a", then "a/b", by cutting the copy at each '/'
        for (usize i = 1; i < path.len; i++) {
            if (cpath[i] != '/') continue;
            cpath[i] = '\0';
            if (c.mkdir(cpath, mode) < 0 && errno != EEXIST)
                return Err(Error.from_errno());
            cpath[i] = '/';
        }
    }

    if (c.mkdir(cpath, mode) == 0) return Ok();
    if (errno == EEXIST && exist_ok && is_dir(cpath)) return Ok();
    return Err(Error.from_errno());
}

static bool is_dir(const char* p) {
    struct stat st;                        // struct keyword: stat is also a function
    return c.stat(p, &st) == 0 && S_ISDIR(st.st_mode);   // C macro, works as-is
}
```

Calling it:

```c
import <sys/stat>

mkdir("out")?;                                   // RWXR_XR_X, i.e. 0755
mkdir("dir/", mode = RWXRWX___)?;                // readable permissions
mkdir("secret", mode = 0700)?;                   // octal still works
mkdir("out", exist_ok = true)?;                  // no error if it exists
mkdir("build/cache/objects", parents = true)?;   // mkdir -p
```

### 20.7 POSIX wrapper: `mmap`

Returns a slice that knows its own length, so `munmap` needs only that slice. Defaults depend on whether a file is given: anonymous read-write memory without one, a private read-only file mapping with one.

```c
// std/sys/mman.lem  (imported as <sys/mman>)
module sys.mman;

import <sys/mman.h>

/// Maps `length` bytes. Without `fd`: zeroed anonymous memory.
/// With `fd`: the file's contents, read-only and private by default.
Result<byte[]> mmap(usize length,
                    int fd = -1,
                    int prot = (fd < 0) ? PROT_READ | PROT_WRITE : PROT_READ,
                    int flags = (fd < 0) ? MAP_PRIVATE | MAP_ANONYMOUS : MAP_PRIVATE,
                    off_t offset = 0,
                    void* hint = null) {
    if (length == 0) return Err(EINVAL, "mmap: length must be greater than 0");

    void* p = c.mmap(hint, length, prot, flags, fd, offset);
    if (p == MAP_FAILED) return Err(Error.from_errno());
    return Ok(((byte*)p)[0:length]);
}

/// Unmaps a region returned by mmap.
Result<void> munmap(byte[] region) {
    if (c.munmap(region.ptr, region.len) < 0) return Err(Error.from_errno());
    return Ok();
}

/// Changes the protection of a mapped region.
Result<void> mprotect(byte[] region, int prot) {
    if (c.mprotect(region.ptr, region.len, prot) < 0) return Err(Error.from_errno());
    return Ok();
}
```

Calling it:

```c
import <sys/mman>

byte[] scratch = mmap(1 << 20)?;                       // 1 MiB of zeroed memory
defer munmap(scratch);

byte[] file = mmap(size, fd = fd)?;                    // read-only view of a file
byte[] shared = mmap(4096, flags = MAP_SHARED | MAP_ANONYMOUS)?;  // shared with children
```

### 20.8 Program: count lines with `mmap`

Opening, sizing, mapping and cleaning up a file, with every error reported and nothing leaked.

```c
// src/main.lem
import <fcntl>
import <unistd>
import <sys/stat>
import <sys/mman>
import <io>

Result<usize> count_lines(str path) {
    int fd = open(path)?;                       // O_RDONLY by default
    defer close(fd);

    usize size = (usize)fstat(fd)?.st_size;
    if (size == 0) return Ok(0);

    byte[] data = mmap(size, fd = fd)?;          // named argument fd, local variable fd
    defer munmap(data);

    usize lines = 0;
    for (byte b : data)
        if (b == '\n') lines++;
    return Ok(lines);
}

Result<void> main(int argc, char** argv) {
    if (argc != 2) return Err(EINVAL, "usage: lines <file>");
    str path = (str)argv[1];
    println("{}: {} lines", path, count_lines(path)?);
    return Ok();
}
```

If the file does not exist, the program prints `error: No such file or directory` and exits with code 2 (`ENOENT`), with the descriptor already closed.

### 20.9 Program: run a child process

`waitpid` returns a tagged union instead of a status integer, so the `switch` is exhaustive and no `WIFEXITED` macros are needed.

```c
// how <sys/wait> defines the result
enum WaitStatus {
    Exited(int code),
    Signaled(int signal),
    Stopped(int signal),
};

Result<WaitStatus> waitpid(pid_t pid, int options = 0, bool retry_eintr = true);
```

```c
import <unistd>
import <sys/wait>
import <io>

Result<void> main() {
    pid_t pid = fork()?;
    if (pid == 0) {
        execvp("ls", "ls", "-l")?;           // returns only on failure
    }

    switch (waitpid(pid)?) {
        case Exited(code):    println("ls exited with {}", code); break;
        case Signaled(sig):   println("ls killed by signal {}", sig); break;
        case Stopped(sig):    println("ls stopped by signal {}", sig); break;
    }
    return Ok();
}
```

Here `execvp` is declared as `Result<void> execvp(str file, ...str argv)`: the wrapper copies the arguments into a NUL-terminated `char*` array on its own stack before calling `c.execvp`.

## 21. What Lemon leaves out

Each of these was considered and rejected because it would make Lemon larger than C programmers will accept, or hide behaviour they need to see.

| Left out | Why | What to use instead |
| --- | --- | --- |
| Classes, inheritance, virtual methods | the object model is the main thing C programmers do not want | structs, methods, function-pointer tables |
| Exceptions and unwinding | invisible control flow | `Result`, `?`, `defer` |
| Destructors and RAII | code running on scope exit without being written there | `defer` |
| Garbage collection, reference counting | hidden cost and pauses | allocators, arenas |
| Borrow checker, lifetimes | the largest complexity cost in Rust | slices, debug checks, `DebugAllocator` |
| Closures that capture | they need hidden allocation | function pointer + `void* ctx` |
| `async`/`await`, coroutines | a second execution model to learn | threads, `poll`, event loops as libraries |
| Compile-time code execution | Zig's hardest feature, large compiler cost | generics, constants, `#if` |
| Hygienic or AST macros | a second language inside the language | generics, file-scoped `#define` |
| Template specialisation, variadic generics | C++ template complexity | constraints, `...any` |
| Distinct (strong) typedefs | `typedef` and `using` stay plain aliases, as in C and C++ | a one-field struct |
| Implicit `from` conversions | conversions that run code must be visible | explicit casts |
| Iterator protocols | abstraction over a `for` loop | `for (x : slice)`, plain `for` |
| Runtime reflection | binary size and hidden metadata | the type tag in `any` only |
| A hand-written code generator | years of work for worse output | C and LLVM backends (section 2) |
| Central package registry (for now) | infrastructure before users | Git dependencies |
| Windows support (for now) | the POSIX layer is the point | Linux, macOS, BSDs firststr name = argc > 1 ? str.from\_cstr(argv\[1\]) : "world"; |
