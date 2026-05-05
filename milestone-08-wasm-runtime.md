# Milestone 8 — WASM Runtime Experiment

## Context

MosaicOS explores WebAssembly as a portable, sandboxed application runtime. A WASM module runs inside the container infrastructure from Milestone 7 but uses the WASM runtime instead of the native runtime. The runtime exposes a controlled set of system APIs — applications cannot access anything not explicitly provided.

## Objective

Integrate a WASM engine as a MosaicOS service. Run a WASM module that uses controlled system APIs (logging, basic I/O). Validate that a WASM module cannot escape its sandbox. Document the WASM API surface.

## Prerequisites

- Milestone 7 complete: container-manager, namespace isolation, and gui-proxy work
- The `Translator` trait and container manifest format support `runtime: wasm`

## Deliverables

### 1. WASM runtime service

Location: `runtimes/wasm-runtime/`

Language: Rust

Choose one of the following embedded WASM engines (in order of preference for embedded/no-std environments):
- **wasm3** — interpreter, minimal dependencies, C library with Rust bindings
- **wasmi** — pure Rust WASM interpreter, no unsafe, good for embedded
- **wasmtime** — higher performance, more complex build

For MosaicOS, prefer `wasmi` if it builds against L4Re's userland. Use `wasm3` as fallback.

The WASM runtime service:
- Starts as a user-space service managed by the service manager
- Accepts `RunModule` IPC requests from the container-manager
- Loads and executes a WASM binary
- Provides the MosaicOS WASM API to the module (see section 3)
- Reports module exit code and any trapped errors back to the container-manager

IPC interface (`sdk/ipc-api/wasm.rs`):
```rust
enum WasmRequest {
    RunModule {
        module_data: SharedMemCap,  // shared memory containing the .wasm binary
        module_size: u32,
        args: [[u8; 64]; 8],        // argv, max 8 args
        env: [[u8; 128]; 8],        // environment, max 8 entries
        capabilities: WasmCapSet,
    },
    // → WasmRunResult { exit_code: i32, error: Option<WasmError> }

    GetApiVersion,
    // → u32 (current WASM API version)
}

struct WasmCapSet {
    can_log: bool,
    can_read_args: bool,
    can_exit: bool,
}
```

### 2. MosaicOS WASM API — mosaic_wasm_api v0.1

Location: `sdk/wasm-api/`

The WASM API is the set of host functions imported by WASM modules to interact with MosaicOS. It is versioned and minimal.

All host functions are in the `mosaic` module namespace:

```
mosaic::log(level: i32, ptr: i32, len: i32) → void
  Sends a log message to logd via the runtime's IPC connection.
  level: 0=debug, 1=info, 2=warn, 3=error

mosaic::get_arg(index: i32, buf_ptr: i32, buf_len: i32) → i32
  Copies argument at `index` into WASM memory at `buf_ptr`.
  Returns actual length, or -1 if index out of range.

mosaic::get_env(key_ptr: i32, key_len: i32, val_ptr: i32, val_len: i32) → i32
  Copies environment value for key into WASM memory.
  Returns actual length, or -1 if key not found.

mosaic::exit(code: i32) → void
  Terminates the WASM module with the given exit code.

mosaic::time_ms() → i64
  Returns milliseconds since system boot.
```

Document the API in `sdk/wasm-api/README.md`. This is the contract all WASM applications target.

Provide a WASM API binding library for Rust targeting WASM:
Location: `sdk/wasm-api/rust-bindings/`

```rust
// Usage in a WASM module written in Rust:
use mosaic_wasm::prelude::*;

#[no_mangle]
pub extern "C" fn _start() {
    mosaic_wasm::log(LogLevel::Info, "hello from WASM");
    mosaic_wasm::exit(0);
}
```

### 3. hello-wasm — first WASM module

Location: `runtimes/experiments/hello-wasm/`

Target: `wasm32-unknown-unknown`

A minimal WASM module that:
- Uses `mosaic::log` to print "MosaicOS Lab: hello from WASM" at info level
- Uses `mosaic::get_arg(0)` to read the first argument and log it
- Calls `mosaic::exit(0)`

Build using `cargo build --target wasm32-unknown-unknown --release`

The resulting `.wasm` binary is the module loaded by the runtime service.

### 4. Container manifest for WASM

Extend the container manifest to support `runtime: wasm`:

```yaml
containers:
  hello-wasm:
    runtime: wasm
    command: /apps/hello.wasm
    wasm:
      api_version: 1
      capabilities:
        can_log: true
        can_read_args: true
        can_exit: true
    network:
      mode: none
    mounts: []
```

The container-manager detects `runtime: wasm` and sends a `WasmRequest::RunModule` to the WASM runtime service instead of spawning a native task.

### 5. Sandbox validation

Verify that a WASM module cannot escape its sandbox:

Create a malicious test module `runtimes/experiments/escape-test-wasm/` that attempts:
- Calling a host function not in `WasmCapSet` (e.g., a made-up `mosaic::disk_write`)
- Accessing memory outside its linear memory bounds
- Infinite loop (test timeout enforcement)

Expected behavior:
- Undeclared host function import → module fails to instantiate with `UnknownImport` error
- Out-of-bounds memory → WASM trap, module terminates with error
- Infinite loop → timeout after a configurable `max_execution_ms` returns `Timeout` error

Document results in `runtimes/experiments/escape-test-wasm/RESULTS.md`.

### 6. mosaic-run WASM support

Extend `mosaic-run` to support WASM containers:

```
mosaic-run hello-wasm
```

Output on serial:
```
[container-manager] starting hello-wasm (runtime: wasm)
[wasm-runtime] loading module /apps/hello.wasm (1234 bytes)
[wasm-runtime] instantiating with api_version=1
[INFO] [hello-wasm] MosaicOS Lab: hello from WASM
[container-manager] hello-wasm exited with code 0
```

### 7. Documentation

`docs/wasm-runtime.md` — document:
- Why WASM as an application runtime
- The MosaicOS WASM API surface (all host functions)
- How to build a WASM application targeting MosaicOS
- Sandbox guarantees and limitations
- Performance characteristics vs native runtime
- Known limitations and future directions

## File layout

```
runtimes/
├── wasm-runtime/
│   └── src/
│       ├── main.rs
│       ├── engine.rs          (wasm engine wrapper)
│       ├── host_api.rs        (mosaic host functions)
│       └── sandbox.rs         (capability enforcement)
└── experiments/
    ├── hello-wasm/
    │   └── src/main.rs        (target: wasm32-unknown-unknown)
    └── escape-test-wasm/
        ├── src/main.rs
        └── RESULTS.md

sdk/
└── wasm-api/
    ├── README.md
    └── rust-bindings/
        ├── src/lib.rs
        └── Cargo.toml

docs/
└── wasm-runtime.md
```

## Architectural constraints

- The WASM runtime service is a regular MosaicOS service. It does not have special privileges beyond what a native service has.
- WASM modules run inside the runtime process's memory space (not as separate L4Re tasks). Isolation is enforced by the WASM engine, not the kernel.
- The WASM API is the complete and only interface available to WASM modules. No POSIX, no syscalls, no direct memory-mapped I/O.
- `WasmCapSet` must be enforced: if `can_log: false`, the `mosaic::log` host function must return an error or trap.
- WASM modules must have a configurable execution timeout. An infinite loop must not hang the runtime service.
- The runtime service must be able to run multiple WASM modules sequentially (not concurrently at this milestone).

## Completion criteria

This milestone is complete when:

- [ ] WASM runtime service starts and registers with mosaic-init
- [ ] `hello-wasm` module loads, executes, logs its message, and exits with code 0
- [ ] Log output appears on serial: `[INFO] [hello-wasm] MosaicOS Lab: hello from WASM`
- [ ] `mosaic-run hello-wasm` works end-to-end
- [ ] Escape test: undeclared host function causes instantiation failure (not a crash)
- [ ] Escape test: out-of-bounds memory access causes a WASM trap (not a runtime crash)
- [ ] Escape test: infinite loop is terminated after `max_execution_ms`
- [ ] Escape test results documented in `RESULTS.md`
- [ ] `docs/wasm-runtime.md` is complete

## Notes for the AI

- Start with `wasmi` for pure Rust simplicity. If it does not build against L4Re's environment, fall back to wasm3 via its C FFI.
- The WASM API is intentionally tiny. Resist adding more functions. The goal is to validate the mechanism, not to build a complete runtime API.
- Host function enforcement: when the WASM engine calls a host function, check the capability set before executing. Return a WASM trap (i32 error code) if the capability is not granted.
- For the execution timeout, use a separate timer thread or L4Re timer capability that kills the running module. Keep this simple: a cooperative timeout via instruction count is acceptable if the engine supports it.
- The `wasm32-unknown-unknown` target produces a bare WASM binary with no WASI. This is intentional — MosaicOS uses its own API, not WASI.
- Consider implementing a tiny `mosaic_wasm::println!` macro in the Rust bindings that calls `mosaic::log` — makes hello-wasm much more ergonomic to write.
- The escape test is important. Document what you expected vs what actually happened. This is research — honest results are more valuable than passing tests.
