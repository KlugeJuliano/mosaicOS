# MosaicOS App Model

**Version:** 0.1 - pre-Milestone 6
**Status:** Design document - authoritative reference for code generation
**Depends on:** `docs/graphics-stack.md`, `docs/recovery.md`

## 1. Philosophy

The MosaicOS app model is a synthesis of three proven ideas applied to a microkernel context:

- **FreeBSD Jails** - isolation by default, capabilities granted explicitly, not revoked selectively
- **OSTree** - content-addressed storage with deduplication via hard links; "git for binaries"
- **Docker Compose** - app + dependencies declared as an atomic unit in a single manifest

The user experience targets macOS simplicity: install, run, remove - no residue, no conflicts, no dependency hell. The implementation beneath that is architecturally closer to FreeBSD + NixOS than to Linux + Flatpak.

### 1.1 Why Not Flatpak

Flatpak solves the same problems but on the wrong foundation for MosaicOS:

- Flatpak isolation uses Linux namespaces - opt-in, implemented in userspace, bypassable
- Flatpak portals use D-Bus - an extra daemon between apps and system services
- Flatpak assumes a Linux kernel ABI

MosaicOS isolation uses L4Re capabilities - mandatory, kernel-enforced, not bypassable. An app without a capability for the network service cannot reach the network; there is no syscall to intercept, no policy to misconfigure. The "jail" is the L4Re address space itself.

Flatpak does use OSTree internally for storage and deduplication. MosaicOS arrives at the same storage conclusion independently and for the same reasons.

### 1.2 Why Not Docker

Docker containers are designed for services, not desktop apps. They have no model for GUI integration, no concept of a compositor surface, and their storage model (tar layers) is less efficient than content-addressed file deduplication. Docker Compose is the right inspiration for the declaration format, not for the runtime model.

## 2. Core Concepts

### 2.1 MosaicBundle

A MosaicBundle is the unit of distribution. It is not an archive or an image; it is a manifest that references content-addressed objects in the MosaicStore.

```text
apps/mosaic-editor/
├── app.mosaic        ← the manifest (TOML)
└── (no binaries here - binaries live in the store)
```

A bundle contains no binaries directly. It contains SHA256 references to objects that the `mosaic-store` service will fetch, verify, and hard-link into the deployment.

### 2.2 MosaicStore

The store is a content-addressed object repository inspired by OSTree and Git. Every file is stored exactly once, identified by its SHA256 hash. Multiple apps sharing a library version share a single file on disk via hard links.

```text
/mosaic/store/
├── objects/
│   ├── ab/cdef1234abcdef...   ← file identified by SHA256
│   ├── 12/3456abcdef1234...
│   └── ...
├── refs/
│   ├── apps/
│   │   ├── mosaic-editor/1.2.0      ← points to a deployment tree
│   │   └── mosaic-terminal/0.9.1
│   └── runtimes/
│       ├── mosaic-native/1
│       └── linux-compat/1
└── deployments/
    └── mosaic-editor-1.2.0/
        ├── bin/editor              ← hard link → objects/ab/cdef...
        ├── lib/libfont.so          ← hard link → objects/12/3456...
        └── lib/libui.so            ← hard link → objects/...
```

Key properties:

- **Deduplication is structural, not optional.** Two apps referencing the same SHA256 object share one file.
- **Installs are atomic.** The deployment directory is constructed completely before being registered as active. A failed install leaves no partial state.
- **Removes are clean.** Removing an app deletes its deployment directory (hard links only) and runs GC on unreferenced objects. No residue elsewhere in the filesystem.
- **Updates download deltas.** The store compares local objects against the remote ref and fetches only missing SHA256 objects.

### 2.3 mosaic-jail Service

`mosaic-jail` is a MosaicOS service (`services/mosaic-jail/`) responsible for launching apps in isolated L4Re address spaces. It is the runtime enforcer of the manifest's capability declarations.

When launching an app, `mosaic-jail`:

1. Creates a new L4Re task (address space + thread).
2. Mounts the deployment filesystem tree (read-only, via L4Re VFS).
3. Creates writable mount points for declared writable paths.
4. Requests capabilities from `service-manager` for each declared service.
5. Passes only those capabilities to the new task - nothing else.
6. Registers the running app with `service-manager` for monitoring and recovery.

The app has no way to acquire capabilities beyond what the manifest declared. There is no ambient authority, no `sudo`, no capability escalation path.

### 2.4 Runtimes

A runtime is a versioned set of shared libraries and ABIs that apps can depend on. Runtimes are stored in the MosaicStore like any other objects.

**`mosaic-native/1`** - the native MosaicOS runtime. Includes `gui-sdk`, `ipc-api`, and standard Rust/libc for MosaicOS. Apps written for MosaicOS use this runtime.

**`linux-compat/1`** - the Linux compatibility runtime. Provides a glibc-compatible environment for apps ported from Linux. Requires the Linux personality service (Milestone 9+). Not available in M5/M6.

Runtimes are themselves versioned objects in the store. Multiple runtime versions coexist without conflict. An app pinning `mosaic-native/1` continues to work after `mosaic-native/2` is released.

## 3. The App Manifest

The manifest (`app.mosaic`) is a TOML file that completely describes the app and everything it needs to run. It is the single source of truth for the `mosaic-jail` launcher and the `mosaic-store` installer.

```toml
[app]
name        = "mosaic-editor"
version     = "1.2.0"
runtime     = "mosaic-native/1"
entry       = "bin/editor"
description = "A text editor for MosaicOS"

[store]
objects = [
  "sha256:ab12cd34ef56gh78ij90kl12mn34op56qr78st90uv12wx34yz560001",
  "sha256:ef34gh56ij78kl90mn12op34qr56st78uv90wx12yz34ab56cd78ef01",
  "sha256:ij56kl78mn90op12qr34st56uv78wx90yz12ab34cd56ef78gh90ij02",
]

[sandbox.mounts]
store = [
  { object = "sha256:ab12cd...", path = "/bin/editor" },
  { object = "sha256:ef34gh...", path = "/lib/libfont.so" },
  { object = "sha256:ij56kl...", path = "/lib/libui.so" },
]
writable = [
  "/home/",
  "/tmp/",
]

[capabilities]
gui     = true
network = false
audio   = false
camera  = false

ipc = [
  "clipboard-server",
  "theme-server",
]

[capabilities.filesystem]
home    = true
tmp     = true
system  = false
```

### 3.1 Capability Enforcement

When `mosaic-jail` reads `capabilities.gui = true`, it requests the compositor capability from `service-manager` and passes it to the app task. The app can then call `CreateWindow` etc. via IPC.

When `capabilities.network = false` or absent, no network capability is passed to the task. The app has no file descriptor, no capability handle, no IPC endpoint for the network service. There is no kernel call available to bypass this; the capability simply does not exist in the task's capability space.

This is structurally different from Linux seccomp, which blocks syscalls, or Flatpak portals, which intercept D-Bus calls. In MosaicOS, denied resources are absent, not blocked.

## 4. mosaic-store Service

`mosaic-store` (`services/mosaic-store/`) manages the object store and handles install, update, remove, and GC operations. It exposes an IPC interface consumed by `mosaicctl` and a future graphical app store.

### 4.1 IPC Interface

```rust
pub enum StoreRequest {
    Install { manifest_url: [u8; 512] },
    Remove { app_name: [u8; 64] },
    UpdateAll,
    Update { app_name: [u8; 64] },
    Launch { app_name: [u8; 64] },
    ListInstalled,
    GarbageCollect,
    Stats,
}

pub struct AppInfo {
    pub name:    [u8; 64],
    pub version: [u8; 32],
    pub runtime: [u8; 32],
    pub status:  AppStatus,
}

pub enum AppStatus {
    Installed,
    Running { pid: u32 },
    UpdateAvailable { new_version: [u8; 32] },
}
```

### 4.2 Install Flow

```text
mosaicctl install https://store.mosaicos.dev/apps/mosaic-editor/1.2.0/app.mosaic
        │
        ▼
mosaic-store fetches manifest → parses [store.objects]
        │
        ▼
for each SHA256 object:
  if /mosaic/store/objects/<sha256> exists → skip (already deduped)
  else → fetch from remote, verify SHA256, write to store
        │
        ▼
construct /mosaic/store/deployments/mosaic-editor-1.2.0/
  via hard links from store objects (atomic: rename into place)
        │
        ▼
write /mosaic/store/refs/apps/mosaic-editor/1.2.0 → deployment path
        │
        ▼
register app with service-manager
```

### 4.3 Update Flow

```text
mosaic-store fetches remote ref for app → new version manifest
        │
        ▼
diff new [store.objects] against local store
        │
        ▼
fetch only missing objects (delta download)
        │
        ▼
construct new deployment directory (hard links)
        │
        ▼
atomic swap: rename new deployment, update ref
  (old deployment stays on disk until GC or explicit remove)
        │
        ▼
running instance continues with old deployment until next launch
next launch uses new deployment
```

### 4.4 Garbage Collection

```text
mosaic-store GC:
  collect all SHA256 objects referenced by any ref in /mosaic/store/refs/
  scan /mosaic/store/objects/ for objects with link count == 1
    (link count == 1 means only the store's own entry, no deployment references it)
  delete unreferenced objects
```

## 5. Filesystem Isolation per App

Each app sees its own filesystem namespace inside the jail. The structure is:

```text
/jail/                    ← jail root (read-only)
├── bin/                  ← app binaries (hard links from store)
├── lib/                  ← app libraries (hard links from store)
├── runtime/              ← runtime libraries (hard links from store)
│   └── mosaic-native/1/
│       ├── libgui-sdk.so
│       └── libipc-api.so
├── home/                 ← app-scoped writable home (not the system home)
│   └── mosaic-editor/    ← isolated per app, not shared between apps
└── tmp/                  ← writable tmp, cleared on app exit
```

**App-scoped home:** each app has its own home directory, isolated from other apps and from the system. `mosaic-editor` cannot read files written by `mosaic-terminal`. This is the iOS sandbox model: each app has its container, not a shared user home.

**Cross-app file sharing:** happens via the `filesystem-server` portal, explicitly. An app requests access to a user-chosen file via a file picker IPC call. The user sees the picker, selects a file, and `filesystem-server` grants a scoped capability for that specific file. No app can read another app's files without explicit user action.

## 6. User-Facing Experience

The entire model above is invisible to the user. From their perspective:

```text
# Install
mosaicctl install mosaic-editor
→ "Installing mosaic-editor 1.2.0..."
→ "Downloaded 3 objects (1 already cached)"
→ "mosaic-editor installed."

# Run
mosaicctl launch mosaic-editor
→ window appears on screen

# Update
mosaicctl update
→ "mosaic-editor: 1.2.0 → 1.3.0 (2 new objects)"
→ "All apps up to date."

# Remove
mosaicctl remove mosaic-editor
→ "mosaic-editor removed."
→ "Freed 4.2 MB (2 objects still in use by other apps)"
```

A future graphical app store (`apps/mosaic-store-ui/`) wraps these same IPC calls in a macOS App Store-style interface.

## 7. File Layout

```text
services/
├── mosaic-jail/
│   ├── Cargo.toml
│   └── src/
│       ├── main.rs         # IPC server, launch coordination
│       ├── manifest.rs     # app.mosaic parser
│       ├── jail.rs         # L4Re task creation, capability injection
│       └── mount.rs        # VFS mount construction from manifest
└── mosaic-store/
    ├── Cargo.toml
    └── src/
        ├── main.rs         # IPC server
        ├── store.rs        # object store operations (fetch, verify, GC)
        ├── deployment.rs   # deployment construction via hard links
        ├── refs.rs         # ref management (install, update, remove)
        └── network.rs      # HTTP fetch for objects and manifests

sdk/ipc-api/
└── store.rs                # StoreRequest, AppInfo, AppStatus

shell/
└── mosaicctl/
    └── src/
        └── commands/
            └── store.rs    # install, remove, update, launch, list subcommands

/mosaic/store/              # runtime store root
├── objects/                # content-addressed files (SHA256)
├── refs/                   # named pointers to deployments
└── deployments/            # active deployment trees (hard links)
```

## 8. Milestone Targeting

This feature is not part of Milestone 5. The prerequisites are:

- M5 complete: compositor, display-server, input-server, mosaic-hello running
- `filesystem-server` with VFS namespace support per process
- `service-manager` extended with `launch_app(manifest)` distinct from `launch_service()`
- Network service available for fetching objects

**Target:** Milestone 6 or 7, after the graphical stack is stable.

For earlier milestones, apps are launched directly via `service-manager` without the jail/store layer. The manifest format and store architecture should be documented now to ensure `service-manager` and `filesystem-server` are designed with the jail model in mind.

## 9. Out of Scope (Initial Implementation)

- Cryptographic signing of manifests and objects (planned post-M7)
- Delta compression for object downloads (OSTree static deltas concept - post-M8)
- `linux-compat/1` runtime (requires Linux personality - Milestone 9)
- Graphical app store UI
- App permissions UI (capability grant/revoke by user at runtime)
- Sandboxed IPC between apps (cross-app communication beyond shared portals)
- App auto-update in background

## 10. Inspirations and Prior Art

| Concept | Inspired by | What MosaicOS does differently |
|---|---|---|
| Content-addressed storage | OSTree, Git, Nix | Applied to app binaries on a microkernel, not a Linux base system |
| Jail isolation | FreeBSD Jails | Implemented via L4Re capabilities instead of kernel namespace primitives |
| Manifest declaration | Docker Compose, Flatpak | Single TOML file, no layered images, no OCI format complexity |
| App-scoped home | iOS App Sandbox | Enforced by filesystem-server capability, not by MAC policy |
| Portal-based file access | Flatpak portals, macOS NSOpenPanel | IPC call to filesystem-server returns a scoped capability, not a path |
| Runtime versioning | Flatpak runtimes | Stored as regular objects in the same store, no special treatment |

This document is the authoritative design reference for the MosaicOS app model. All implementation of `mosaic-jail`, `mosaic-store`, and related `service-manager` extensions should follow this spec. Decisions not covered here should prefer the simplest correct solution and be documented in the relevant source file.
