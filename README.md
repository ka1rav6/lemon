# Lemon

**C with the boilerplate removed.** Same syntax, same memory model, same ABI — plus generics, methods, slices, `Result`, `defer`, named arguments and headerless modules.

Lemon's reason to exist is its **POSIX layer**: every common system call wrapped with `Result` errors and slices instead of pointer-plus-length, while the raw C call stays one import away.

If you can read C, you can read Lemon today. The new things are `T[]`, `Result<T>`, `?`, `defer`, `<T>`, `x.method()`, `f(name = value)`, `u64`-style integer names and `import`.

```c
import <io>

int main(int argc, char** argv) {
    str name = argc > 1 ? (str)argv[1] : "world";
    println("Hello, {}! You passed {} argument(s).", name, argc - 1);
    return 0;
}
```

Source files are `.lem`; the compiler, build system and package manager are all the `lem` command.

## Status

**Pre-alpha — nothing compiles yet.** This repository currently holds the design and the plan. The compiler is being written in C17, starting from milestone M0. First usable release is M10 (`v0.5.0`).

| Document | What it is |
| --- | --- |
| [DESIGN.md](DESIGN.md) | The language specification |
| [ROADMAP.md](ROADMAP.md) | Milestone-by-milestone build order and status |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| [AI_USAGE.md](AI_USAGE.md) | AI policy for contributions |

## Design rules

1. **If C already does it well, Lemon does it the C way.**
2. **Nothing hidden.** No hidden allocation, no hidden control flow, no destructors, no GC.
3. **Every new feature removes boilerplate or prevents a common bug.**
4. **Zero-cost by default.** Generics monomorphized, methods are plain functions, checks vanish in release.
5. **C interop in both directions**, with no binding generator.

Explicitly out of scope for 1.0: classes, inheritance, exceptions, RAII, GC, borrow checking, capturing closures, async/await, compile-time execution, macros, runtime reflection and Windows.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) and [AI_USAGE.md](AI_USAGE.md) first. This codebase aims to stay readable by beginners: plain C17, one Makefile, one header per subsystem.

## License

MIT — see [LICENSE](LICENSE).
