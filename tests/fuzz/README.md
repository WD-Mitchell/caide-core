# caide_core fuzz harnesses

libFuzzer harnesses that exercise the public C ABI for two of the most
state-heavy subsystems in `caide_core`:

| Harness | Surface |
|---|---|
| `fuzz_command_executor` | `caide_command_validate` + `caide_command_execute` over fuzz-derived `(type, params_json, target_ref)` triples |
| `fuzz_history` | sequences of `caide_command_execute` / `caide_document_undo` / `caide_document_redo`, asserting undo+redo identity, history-count bounds, and "empty history" semantics |

Each harness:

* Validates input bytes and **never aborts on expected error codes** — only
  on UB, sanitizer findings, asserts, or violated invariants.
* Cleans up every shape/document/context it creates so leak-checking
  sanitizers stay quiet.

## Prerequisites

* **Clang** (Apple Clang or mainline LLVM; mainline preferred for real fuzzing)
* For real libFuzzer runs: a Clang that ships `libclang_rt.fuzzer_*`
  (mainline LLVM, or `brew install llvm` on macOS — Apple Clang **does
  not** ship libFuzzer)
* AddressSanitizer + UBSan are enabled by default in the build flags

## Build modes

The fuzz subsystem has two CMake options:

| Option | Default | Purpose |
|---|---|---|
| `CAIDE_BUILD_FUZZ` | `OFF` | Add the fuzz harnesses + `fuzz-smoke` target to the build |
| `CAIDE_FUZZ_STANDALONE` | `OFF` | Replace libFuzzer with a self-driving `main()` (no instrumentation). Use on platforms without libFuzzer (e.g. macOS Apple Clang) for harness smoke. |

`CAIDE_FORCE_STUB=ON` is useful when you don't have OpenCASCADE installed —
the harnesses don't depend on the OCCT path and run faster in stub mode.

### Local: real libFuzzer (Linux, or macOS with Homebrew LLVM)

```sh
# macOS: use Homebrew LLVM
brew install llvm
export CC=/opt/homebrew/opt/llvm/bin/clang
export CXX=/opt/homebrew/opt/llvm/bin/clang++

cmake -S . -B build_fuzz \
    -DCAIDE_BUILD_FUZZ=ON \
    -DCAIDE_BUILD_TESTS=OFF \
    -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX
cmake --build build_fuzz --target fuzz-smoke
```

`fuzz-smoke` runs **each** harness for **60 seconds** (capped at 500k runs)
with a persistent corpus directory under `build_fuzz/tests/fuzz/corpus/`.

To do a longer run:

```sh
./build_fuzz/tests/fuzz/fuzz_command_executor -max_total_time=600 build_fuzz/tests/fuzz/corpus
```

### Local: macOS Apple Clang (no libFuzzer)

```sh
cmake -S . -B build_fuzz \
    -DCAIDE_BUILD_FUZZ=ON \
    -DCAIDE_FUZZ_STANDALONE=ON \
    -DCAIDE_BUILD_TESTS=OFF \
    -DCAIDE_FORCE_STUB=ON \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

cmake --build build_fuzz --target fuzz-smoke
```

The standalone driver feeds ~200 deterministic + pseudo-random inputs per
harness and exits 0 only if no input crashes — this is just smoke; it
does **not** find new bugs the way libFuzzer's coverage-guided search
does.

### CI

Wiring fuzz into CI is **out of scope** for this Bead. Suggested follow-up:

* Add a job to `.github/workflows/` that runs on Ubuntu with mainline Clang,
  configures with `CAIDE_BUILD_FUZZ=ON CAIDE_FORCE_STUB=ON`, then runs
  `cmake --build build_fuzz --target fuzz-smoke`. Cache the corpus
  directory across runs for coverage continuity.

## Reproducing a crash

libFuzzer dumps reproducers as `crash-<sha1>` files in the working
directory. To re-run:

```sh
./build_fuzz/tests/fuzz/fuzz_command_executor crash-abc123
```

For the standalone driver, pass an integer seed to reproduce the same
pseudo-random sequence: `./fuzz_history 42`.

## Adding a new harness

1. Drop `fuzz_<thing>.cpp` into this directory.
2. Append `#include "standalone_main.inc"` at the end so it builds under
   `CAIDE_FUZZ_STANDALONE`.
3. Add `caide_add_fuzz(fuzz_<thing> fuzz_<thing>.cpp)` to
   `CMakeLists.txt` and (optionally) chain it into `fuzz-smoke`.
