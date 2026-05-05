# Milestone 9 — Linux Personality Research

## Context

The long-term vision of MosaicOS includes the ability to run existing Linux applications in an isolated personality layer on top of the microkernel. This milestone is a research experiment, not a production feature. The goal is to understand the mechanics of Linux ABI compatibility — ELF loading, syscall translation, and process model — well enough to run a minimal static Linux binary.

This is the most complex and open-ended milestone. The deliverables are part implementation, part research documentation.

## Objective

Study ELF loading, implement a minimal Linux syscall translation layer, and attempt to run a small statically-linked Linux binary (e.g., a hello-world compiled with musl libc). Document every finding, limitation, and decision.

## Prerequisites

- Milestone 7 complete: container infrastructure exists, native runtime works
- Milestone 8 complete: WASM runtime experiment provides contrast for runtime design decisions
- Strong understanding of L4Re task creation and capability model from all previous milestones

## Deliverables

### 1. Research document: Linux ABI study

Location: `docs/linux-personality-research.md`

Before writing any code, document:

**ELF binary format:**
- ELF header structure: magic, class (32/64-bit), entry point, program headers
- Segment types: PT_LOAD, PT_INTERP, PT_DYNAMIC, PT_GNU_STACK
- Static vs dynamic linking — why static-only is the scope of this milestone
- Relocation types relevant for static binaries

**Linux process model:**
- Virtual address space layout: text, data, BSS, heap, stack, vDSO, kernel space
- Auxiliary vector (auxv): what information the kernel passes to a new process
- argc, argv, envp layout on the initial stack
- Stack alignment requirements (16-byte on x86-64)

**Linux syscall ABI on x86-64:**
- Syscall instruction: `syscall` opcode
- Register convention: rax=number, rdi/rsi/rdx/r10/r8/r9=args, rax=return
- Error convention: return value -errno on failure (values -1 to -4095)
- Key syscalls needed to run a minimal hello-world:
  - `write` (1): write bytes to a file descriptor
  - `exit_group` (231) / `exit` (60): terminate process
  - `brk` (12): adjust heap break
  - `mmap` (9): map memory (for stack setup)
  - `arch_prctl` (158): set FS register for thread-local storage

**vDSO:**
- What the vDSO is and why glibc/musl use it
- Whether to provide a stub vDSO or skip it
- Impact on musl-linked binaries

### 2. ELF loader

Location: `runtimes/linux-personality/elf-loader/`

Language: Rust

A library that:
- Parses an ELF64 binary from a byte slice
- Validates the ELF header (magic, class=64, machine=x86-64)
- Reads all PT_LOAD segments and their memory mappings (vaddr, memsz, filesz, flags)
- Reports the entry point address
- Rejects dynamic binaries (PT_INTERP present → error `DynamicNotSupported`)

```rust
pub struct ElfBinary<'a> {
    data: &'a [u8],
}

impl<'a> ElfBinary<'a> {
    pub fn parse(data: &'a [u8]) -> Result<Self, ElfError>
    pub fn entry_point(&self) -> u64
    pub fn segments(&self) -> impl Iterator<Item = LoadSegment>
    pub fn is_static(&self) -> bool
    pub fn arch(&self) -> ElfArch
}

pub struct LoadSegment {
    pub vaddr:   u64,
    pub filesz:  u64,
    pub memsz:   u64,
    pub offset:  u64,
    pub flags:   SegmentFlags,  // Read, Write, Execute
}
```

Tests for the ELF loader (pure Rust unit tests, no L4Re needed):
- Parse a valid static x86-64 ELF → extract entry point and segments
- Reject a 32-bit ELF
- Reject a dynamic ELF
- Reject a truncated ELF

### 3. Linux personality service

Location: `runtimes/linux-personality/`

Language: Rust

A MosaicOS service that runs a static Linux binary inside an L4Re task with syscall translation.

Architecture:
```
[linux-personality service]
    ├── ELF loader: maps binary segments into the task
    ├── Stack builder: sets up argc/argv/envp/auxv on the stack
    ├── Syscall handler: intercepts and translates Linux syscalls
    └── L4Re task: runs the translated binary
```

Syscall interception strategy — choose one:
- **Ptrace-like mechanism**: L4Re allows a parent task to observe and handle exceptions from a child. Linux `syscall` on x86-64 triggers a general protection fault when executed outside kernel mode in an L4Re task — the parent (linux-personality) can catch this and handle it.
- **Binary rewriting**: Before loading, scan for `syscall` instructions and replace with a call into a stub library. Complex but avoids exception handling overhead.

Prefer the exception-based approach for simplicity. Document the chosen strategy and its trade-offs in `docs/linux-personality-research.md`.

**Syscall translation table** — implement these for the first experiment:

| Linux syscall | Number | MosaicOS translation |
|---|---|---|
| `write`       | 1      | If fd=1 or fd=2: route to logd IPC. Otherwise: EBADF. |
| `exit`        | 60     | Terminate the L4Re task, report exit code to linux-personality. |
| `exit_group`  | 231    | Same as exit. |
| `brk`         | 12     | Return current brk (static binaries with musl may not call this). |
| `arch_prctl`  | 158    | If ARCH_SET_FS: store value, return 0. Others: ENOSYS. |
| *(others)*    | any    | Return -ENOSYS. Log the unknown syscall number. |

### 4. Stack builder

Location: `runtimes/linux-personality/stack.rs`

Correctly set up the initial stack for a Linux process:

```
HIGH ADDRESS
┌─────────────────┐
│ environment str │  actual env strings
│ argument strs   │  actual argv strings
├─────────────────┤
│ 0               │  end of envp
│ envp[n]         │  pointers to env strings
│ ...             │
│ envp[0]         │
├─────────────────┤
│ 0               │  end of argv
│ argv[n]         │  pointers to arg strings
│ ...             │
│ argv[0]         │
├─────────────────┤
│ argc            │  number of arguments (i64)
└─────────────────┘ ← rsp (stack pointer on entry)
LOW ADDRESS
```

Auxiliary vector (auxv) placed above envp (before the null terminator of envp):
```
AT_PHDR    → address of ELF program headers in memory
AT_PHNUM   → number of program headers
AT_ENTRY   → entry point address
AT_PAGESZ  → 4096
AT_NULL    → end of auxv
```

Stack must be 16-byte aligned at the point of entry (`rsp % 16 == 0` before the `call` or `jmp` to entry point).

### 5. hello-linux — the test binary

Location: `runtimes/experiments/hello-linux/`

A minimal C program compiled with musl libc as a static binary:

```c
#include <unistd.h>

int main(int argc, char **argv) {
    const char msg[] = "MosaicOS Linux personality: hello\n";
    write(1, msg, sizeof(msg) - 1);
    return 0;
}
```

Build command:
```bash
musl-gcc -static -O2 -o hello-linux hello.c
```

Verify it runs on a normal Linux host before testing on MosaicOS.

If musl-gcc is not available, use any static binary that calls only `write` and `exit`.

### 6. linux-personality container manifest

```yaml
containers:
  hello-linux:
    runtime: linux
    command: /apps/hello-linux
    linux:
      abi: x86_64-static
      syscall_strategy: exception
    network:
      mode: none
    mounts: []
    capabilities: []
```

### 7. Research findings document

Location: `docs/linux-personality-research.md`

After completing the implementation attempt, document:

**What worked:**
- ELF parsing: did it load segments correctly?
- Stack setup: did the binary reach its entry point?
- Syscall interception: were `write` and `exit` intercepted correctly?
- Output: did "MosaicOS Linux personality: hello" appear on serial?

**What did not work:**
- Which syscalls caused unexpected behavior?
- Were there ABI differences from what musl expected?
- Were there L4Re limitations that complicated the implementation?

**Trade-offs discovered:**
- Exception-based vs binary-rewriting for syscall interception
- Complexity of full Linux ABI vs the subset implemented

**Feasibility assessment:**
- Is continuing Linux personality development worthwhile for MosaicOS?
- What would the next 10 syscalls require?
- What is the estimated effort to run a dynamically linked binary?
- What is the estimated effort to run a shell?

**Comparison with WASM runtime (Milestone 8):**
- Isolation guarantees: which is stronger?
- Implementation complexity: which is simpler?
- Application compatibility: which covers more use cases?

### 8. mosaicctl linux subcommand

```
mosaicctl linux status
```

Shows linux-personality service state and any active linux containers.

```
mosaicctl linux syscalls
```

Shows a log of all syscalls intercepted since boot, with counts:
```
SYSCALL        NUMBER   COUNT   LAST_RESULT
write          1        42      0
exit_group     231      1       0
arch_prctl     158      2       0
unknown(999)   999      1       -ENOSYS
```

## File layout

```
runtimes/
├── linux-personality/
│   ├── src/
│   │   ├── main.rs
│   │   ├── service.rs         (MosaicOS service wrapper)
│   │   ├── runner.rs          (ELF load + task creation)
│   │   ├── syscall.rs         (syscall handler and translation)
│   │   └── stack.rs           (initial stack builder)
│   ├── elf-loader/
│   │   ├── src/lib.rs
│   │   └── tests/
│   └── Cargo.toml
└── experiments/
    └── hello-linux/
        └── hello.c

docs/
└── linux-personality-research.md
```

## Architectural constraints

- The Linux personality runs as a MosaicOS service. It does not extend the kernel.
- Linux binaries are executed in an isolated L4Re task. A crash in the Linux binary must not crash the linux-personality service.
- Only static binaries are in scope. Dynamic linking requires a dynamic linker — out of scope.
- Only x86-64 is in scope. No 32-bit compat layer.
- Unknown syscalls return `-ENOSYS`. They must be logged but must not crash anything.
- The linux-personality service must survive a binary that calls an infinite loop of `write`. Implement a per-run resource limit (e.g., max wall-clock time).

## Completion criteria

**Minimum (research milestone):**
- [ ] ELF loader parses a static x86-64 ELF and extracts segments and entry point
- [ ] ELF loader unit tests pass
- [ ] `docs/linux-personality-research.md` documents ELF format, syscall ABI, and auxv in full detail
- [ ] Implementation attempt is made, with findings documented honestly

**Full success:**
- [ ] hello-linux binary loads, executes, and outputs "MosaicOS Linux personality: hello" on serial
- [ ] `write` (fd=1) and `exit_group` syscalls are correctly intercepted
- [ ] An unknown syscall returns -ENOSYS and is logged
- [ ] `mosaicctl linux syscalls` shows the syscall count table
- [ ] Feasibility assessment is written in `docs/linux-personality-research.md`

Note: If the full success criteria prove infeasible within reasonable effort, the minimum criteria plus thorough documentation of why it failed is equally valuable for a research project.

## Notes for the AI

- This is the most uncertain milestone. Approach it as a research experiment, not a feature delivery. Document what you learn as you go.
- Exception-based syscall interception in L4Re: look for `l4_debugger_set_object_name` and exception handler IPC in the L4Re documentation. The mechanism is similar to `ptrace(PTRACE_SYSCALL)` in Linux.
- The ELF loader is the most testable part of this milestone. Write it as a pure library first and test it against known ELF binaries on the host before integrating with L4Re.
- musl libc's hello world may call more syscalls than expected. Use `strace ./hello-linux` on Linux first to see exactly which syscalls are needed.
- Stack alignment is a common source of subtle failures. If the binary segfaults immediately at the entry point, check alignment first.
- If syscall interception proves impossible in L4Re's current form, document the blocker in detail and consider the milestone complete at the research documentation level.
- The feasibility assessment is a genuine research contribution. Be honest about what is hard, what is easy, and what you would do differently.
