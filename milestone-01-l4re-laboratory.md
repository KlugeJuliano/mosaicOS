# Milestone 1 — L4Re Laboratory

## Context

MosaicOS Lab is built on a microkernel foundation. The chosen microkernel is L4Re (using Fiasco.OC as the kernel). Before writing any MosaicOS-specific code, this milestone establishes a reproducible laboratory environment where L4Re boots in QEMU and a basic user-space task runs successfully.

This milestone is purely exploratory and infrastructural. No MosaicOS abstractions are introduced yet.

## Objective

Build L4Re from source, boot it in QEMU, run a hello world task, and document every step so the environment is fully reproducible. Study the IPC mechanism by sending a message between two tasks.

## Prerequisites

- Milestone 0 complete (repository structure and documentation exist)
- LICENSE file present at repository root
- Host machine: Linux x86-64 (or equivalent Docker/VM environment)

## Deliverables

### 1. Build environment

Provide one of the following (in order of preference):
- `Dockerfile` at `tools/lab/Dockerfile` that installs all required build dependencies and produces a working L4Re build inside the container
- `tools/lab/setup.sh` shell script that installs dependencies on Ubuntu 24.04 and builds L4Re

Required toolchain components:
- L4Re source (https://github.com/kernkonzept/l4re-core or official snapshot)
- Fiasco.OC kernel source
- Cross-compiler or native x86-64 compiler (GCC or Clang)
- QEMU with x86-64 and KVM support (`qemu-system-x86_64`)
- GRUB or equivalent bootloader for L4Re boot image generation

### 2. Build scripts

`tools/lab/build-kernel.sh` — builds Fiasco.OC kernel binary
`tools/lab/build-l4re.sh` — builds L4Re userland
`tools/lab/build-hello.sh` — builds the hello world task
`tools/lab/run-qemu.sh` — boots the full image in QEMU with correct flags

QEMU flags must include:
- `-serial stdio` for serial output visible in terminal
- `-m 256M` minimum memory
- `-nographic` or `-display none` for headless boot in CI
- `-no-reboot` to halt on exit

### 3. Hello world task

Location: `microkernel/experiments/hello/main.c` (or `main.rs` if using Rust)

The task must:
- Print "MosaicOS Lab: hello from L4Re" to the serial console
- Exit cleanly

This is the simplest possible L4Re task — no IPC, no services, just output and exit.

### 4. IPC experiment

Location: `microkernel/experiments/ipc-ping/`

Two tasks communicating via L4Re IPC:
- `sender.c` — sends a string message to the receiver
- `receiver.c` — receives the message and prints it to serial

This experiment validates that the IPC mechanism works before building the service manager on top of it.

Files:
- `microkernel/experiments/ipc-ping/sender.c`
- `microkernel/experiments/ipc-ping/receiver.c`
- `microkernel/experiments/ipc-ping/README.md` — explains how to build and run

### 5. Documentation

`docs/lab-setup.md` — complete step-by-step setup guide covering:
- Host system requirements
- How to build the kernel and userland
- How to run in QEMU
- Expected output for each experiment
- Common errors and how to fix them
- How to add a new task to the L4Re boot configuration

## L4Re technical notes for the AI

L4Re boot configuration uses a `modules.list` file that declares which binaries are loaded at boot. A typical entry looks like:

```
module /path/to/binary
```

The bootstrap task is the first thing that runs after the kernel. It reads the module list and starts the initial tasks.

L4Re IPC uses typed message registers (MRs). A minimal IPC send looks like:

```c
l4_msg_regs_t *mr = l4_utcb_mr();
mr->mr[0] = 42;
l4_ipc_send(target_cap, l4_utcb(), l4_msgtag(0, 1, 0, 0), L4_IPC_NEVER);
```

Capabilities in L4Re are unforgeable references to kernel objects. A task receives capabilities via the bootstrap configuration or by explicit delegation from a parent task.

## Architectural constraints

- No MosaicOS abstractions in this milestone. Pure L4Re code only.
- All experiments must build reproducibly from the scripts provided.
- Serial output is the only I/O mechanism at this stage. No framebuffer, no filesystem.
- Document every assumption about the build environment explicitly.

## Completion criteria

This milestone is complete when:

- [ ] L4Re builds from source using the provided scripts with no manual steps
- [ ] QEMU boots the L4Re image and shows kernel startup messages on serial
- [ ] Hello world task runs and prints the expected message
- [ ] IPC ping experiment: sender sends a message, receiver prints it
- [ ] `docs/lab-setup.md` is complete and accurate — a new contributor can follow it from scratch
- [ ] All build scripts are idempotent (running twice produces the same result)

## Notes for the AI

- Prefer C for experiments at this stage. Rust is acceptable if L4Re Rust bindings are available and the build complexity does not increase significantly.
- Do not introduce any MosaicOS-specific abstractions yet. This milestone is about understanding the foundation, not building on it.
- If L4Re documentation is sparse, check the official L4Re tutorial and the Genode OS framework (which uses the same microkernel) for reference patterns.
- The build system for L4Re uses a custom `make` based system (BID — Build Infrastructure for Developers). Follow BID conventions for new packages.
- Serial output in L4Re uses `printf` via the `l4/util/util.h` header or the `LOG` server. Prefer `printf` for simplicity in experiments.
- Keep experiments minimal. The goal is to validate the environment, not to build features.
