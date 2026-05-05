# Milestone 6 — Translator Prototype

## Context

MosaicOS is inspired by GNU Hurd's concept of translators: user-space services that expose arbitrary resources (cloud storage, databases, network APIs, virtual devices) as part of the filesystem tree. Applications interact with these resources using ordinary file operations.

This milestone implements the first working translator: a virtual filesystem translator that exposes a static in-memory resource as a mountable directory.

## Objective

Define the translator IPC protocol, implement the translator lifecycle manager (integrated into the service manager), implement one concrete translator (static virtual filesystem), and expose it as a directory in the system's namespace.

## Prerequisites

- Milestone 3 complete: service manager handles multiple services, capability manager works
- Milestone 5 is not required for this milestone (translators are independent of graphics)
- docs/translators.md defines translator concepts and the mount manifest format

## Deliverables

### 1. Translator IPC protocol

Location: `sdk/ipc-api/translator.rs`

The translator protocol defines how mosaic-init mounts translators and how applications access translator-exposed paths.

```rust
// Sent by the namespace manager to a translator at mount time
enum TranslatorRequest {
    Mount { target_path: [u8; 256], config: TranslatorConfig },
    Unmount,
    GetStatus,       // → TranslatorStatus { mounted, path, type }
}

// Sent by applications (via VFS) to an active translator
enum VfsRequest {
    Open  { path: [u8; 256], flags: u32 },  // → FileHandle
    Read  { handle: u32, offset: u64, size: u32 }, // → bytes
    Write { handle: u32, offset: u64, data: [u8; 4096] },
    Close { handle: u32 },
    List  { path: [u8; 256] },              // → Vec<DirEntry>
    Stat  { path: [u8; 256] },              // → FileStat
}

struct DirEntry {
    name: [u8; 256],
    kind: EntryKind,  // File, Directory, Translator
    size: u64,
}

struct FileStat {
    size: u64,
    kind: EntryKind,
    readable: bool,
    writable: bool,
}
```

### 2. Namespace manager

Location: `core/namespace-manager/`

Language: Rust

The namespace manager maintains the system's path-to-translator mapping. It is started by mosaic-init as a core service.

Responsibilities:
- Read `mounts:` from the system manifest at boot
- Start each translator service and send it a `Mount` request
- Route VFS path lookups to the correct translator
- Handle translator failures: unmount path, notify guardian

```rust
pub struct NamespaceManager {
    mounts: HashMap<String, TranslatorCap>,
}

impl NamespaceManager {
    pub fn resolve(&self, path: &str) -> Option<TranslatorCap>
    pub fn mount(&mut self, target: &str, translator: TranslatorCap) -> Result<()>
    pub fn unmount(&mut self, target: &str) -> Result<()>
}
```

IPC interface (for applications and mosaicctl):
```rust
enum NamespaceRequest {
    Resolve { path: [u8; 256] },       // → TranslatorCap or None
    ListMounts,                         // → Vec<MountInfo>
    Mount { target: [u8; 256], translator_service: [u8; 64] },
    Unmount { target: [u8; 256] },
}
```

### 3. translator-sdk v0.1

Location: `sdk/translator-sdk/`

Language: Rust

A library that makes it easy to implement new translators. Every translator uses this SDK rather than implementing the IPC protocol directly.

```rust
pub trait Translator: Send {
    fn mount(&mut self, config: TranslatorConfig) -> Result<()>;
    fn unmount(&mut self) -> Result<()>;
    fn open(&mut self, path: &str, flags: u32) -> Result<FileHandle>;
    fn read(&mut self, handle: FileHandle, offset: u64, buf: &mut [u8]) -> Result<usize>;
    fn write(&mut self, handle: FileHandle, offset: u64, data: &[u8]) -> Result<usize>;
    fn close(&mut self, handle: FileHandle) -> Result<()>;
    fn list(&mut self, path: &str) -> Result<Vec<DirEntry>>;
    fn stat(&mut self, path: &str) -> Result<FileStat>;
}

// Implement this to run a translator:
pub fn run_translator<T: Translator>(translator: T) -> ! { ... }
```

`run_translator` handles the IPC receive loop and dispatches to the trait methods. A new translator only needs to implement the `Translator` trait.

### 4. static-vfs — the first translator

Location: `translators/static-vfs/`

Language: Rust

A translator that exposes a hardcoded in-memory directory tree as a mountable path.

The directory tree it exposes:
```
/info/
├── version     (file: "MosaicOS Lab v0.1")
├── build-date  (file: current build timestamp)
├── kernel      (file: "L4Re / Fiasco.OC")
└── status/
    ├── services  (file: live service count from registry)
    └── uptime    (file: seconds since boot)
```

The `services` and `uptime` files are dynamic: reading them queries the service registry via IPC and returns current data.

Implementation using the translator SDK:
```rust
struct StaticVfsTranslator {
    registry: ServiceRegistryCap,
}

impl Translator for StaticVfsTranslator {
    fn list(&mut self, path: &str) -> Result<Vec<DirEntry>> { ... }
    fn read(&mut self, handle: FileHandle, offset: u64, buf: &mut [u8]) -> Result<usize> { ... }
    // write returns Err(ReadOnly) for all paths
}
```

### 5. System manifest — mount declaration

Add translator mounts to `boot/manifests/development.yaml`:

```yaml
services:
  namespace-manager:
    binary: /system/core/namespace-managerd
    restart: always
    requires:
      - logger

  translator-static-vfs:
    binary: /system/translators/static-vfsd
    restart: on-failure
    requires:
      - namespace-manager
    capabilities:
      - registry.read

mounts:
  - target: /info
    type: translator
    translator: static-vfs
    config: {}
```

### 6. mosaicctl mounts command

Extend `tools/mosaicctl/` with:

```
mosaicctl mounts
```

Output:
```
MOUNT POINT    TRANSLATOR       STATE
/info          static-vfs       mounted
/cloud         s3               unmounted (service not started)
```

```
mosaicctl read /info/version
```

Sends a `VfsRequest::Open` + `VfsRequest::Read` + `VfsRequest::Close` sequence via the namespace manager and prints the result to serial/stdout.

### 7. Integration test

Location: `tools/lab/translator-test.sh`

A script that:
1. Boots the full manifest in QEMU
2. Waits for the translator-static-vfs service to reach `running` state
3. Uses `mosaicctl read /info/version` to read the version file
4. Verifies the output matches the expected string
5. Reads `/info/status/services` and verifies a non-zero count
6. Stops the translator service, verifies the mount is unmounted gracefully

## File layout

```
core/
└── namespace-manager/
    └── src/main.rs

translators/
└── static-vfs/
    └── src/main.rs

sdk/
├── ipc-api/
│   └── translator.rs          (new)
└── translator-sdk/
    ├── src/
    │   ├── lib.rs
    │   └── ipc_loop.rs
    └── Cargo.toml

tools/
├── mosaicctl/
│   └── src/
│       └── mounts.rs          (new)
└── lab/
    └── translator-test.sh     (new)
```

## Architectural constraints

- Applications never talk to translators directly. They go through the namespace manager.
- The namespace manager is the only component that holds translator capabilities.
- Translators must not crash the namespace manager. The namespace manager handles translator failures gracefully (unmount + notify guardian).
- The translator SDK's `run_translator` function owns the IPC loop. Translator implementations must be purely synchronous.
- Write operations to a read-only translator must return a clear error, not silently succeed.
- Translator lifecycle is managed by the service manager. The namespace manager does not start or stop translator processes — it only sends `Mount`/`Unmount` IPC to running translator services.

## Completion criteria

This milestone is complete when:

- [ ] namespace-manager starts and registers with mosaic-init
- [ ] static-vfs translator starts and receives a `Mount` request from namespace-manager
- [ ] `mosaicctl mounts` shows `/info` as mounted
- [ ] `mosaicctl read /info/version` returns "MosaicOS Lab v0.1"
- [ ] `mosaicctl read /info/status/services` returns a non-zero integer
- [ ] Reading a non-existent path returns a clear error
- [ ] Stopping the static-vfs service causes `/info` to unmount gracefully
- [ ] A new translator can be created using the translator-sdk with minimal boilerplate (< 50 lines for a trivial read-only translator)
- [ ] `translator-test.sh` runs end-to-end and exits 0

## Notes for the AI

- The namespace manager is a path prefix router. It does not implement a full VFS — it maps top-level mount points to translator capabilities. Full VFS semantics (permissions, links, etc.) are out of scope for this milestone.
- For the dynamic files in static-vfs (`/info/status/services`, `/info/status/uptime`), generate the content at read time. Do not cache. Keep it simple.
- The translator SDK should handle all IPC machinery. A translator implementor should only write the business logic. If implementing a translator requires understanding L4Re IPC details, the SDK is not doing its job.
- `mosaicctl read` is a debugging tool, not a production interface. Implement it simply: open, read in chunks, close, print to serial.
- The `translator-test.sh` script should use `grep` or exact string comparison on the QEMU serial output. Pipe QEMU serial to a file during the test for easy verification.
- Future translators (s3, sqlite, http) will use the same `Translator` trait. The static-vfs is a template — make sure the SDK is easy to extend.
