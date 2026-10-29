# Contributing to Lemon

Thanks for looking. The compiler is early — see [ROADMAP.md](ROADMAP.md) for what exists and what's next.

## Before you start

- **Read the docs.** [DESIGN.md](DESIGN.md) is the language spec; [ROADMAP.md](ROADMAP.md) is the order of work. A change that contradicts the spec needs the spec changed first.
- **Read [AI_USAGE.md](AI_USAGE.md).** You must understand and be able to defend every line you submit.
- **Open an issue first** for anything larger than a bug fix. Milestones are deliberately sequential; work that jumps ahead of the current milestone is usually not mergeable yet.

## Ground rules for compiler code

The compiler is written in plain C17. Keep it readable by someone learning compilers.

1. **One Makefile, one header per subsystem.** No macro tricks beyond the growable-array helpers.
2. **Arenas for everything the front end allocates.** AST nodes, types, symbols, strings and LIR live in arenas released at exit. No `free()` in the front end.
3. **Sanitizers always on in debug:** `-g -fsanitize=address,undefined -Wall -Wextra -Werror`.
4. **Errors are values.** Diagnostics are collected and the parser recovers at `;` and `}`, so one run reports many errors. `ICE()` is for internal bugs only.
5. **Source locations everywhere.** Every token, AST node and LIR instruction carries `SrcLoc`; every error shows the line with a caret.

## Tests

| Kind | Location | How it runs |
| --- | --- | --- |
| Toolkit unit tests | `tests/base/*.c` | `make test` (sanitizer build), via vendored `tests/greatest.h` |
| Run tests | `tests/ok/NAME.lem` + `NAME.out` | compile, run, diff stdout (and exit code if `NAME.code` exists) |
| Error tests | `tests/err/NAME.lem` | `// error: LINE:COL message` comments must match diagnostics exactly |
| In-language tests | `#test` blocks, `tests/std/` | `lem test` |
| Example programs | `examples/` | built and run in CI |

Run `tests/run.sh` before every commit. CI runs it on every push.

## Definition of done

A change is ready to merge when:

- [ ] It adds at least one test in `tests/ok`, `tests/err` or a `#test` block
- [ ] `tests/run.sh` passes with the sanitizer build
- [ ] Every bug fix includes a test that failed before the fix
- [ ] Diagnostics it adds point at the right line and column
- [ ] Any spec question it raised is recorded in `NOTES.md`
- [ ] The milestone checklist in [ROADMAP.md](ROADMAP.md) is updated

## Pull requests

- One logical change per PR; keep the diff as small as the change allows.
- Imperative commit subjects under ~72 characters (`parser: recover at closing brace`).
- Say in the description what you tested, and disclose AI assistance.

By contributing you agree your work is licensed under the MIT [LICENSE](LICENSE).
