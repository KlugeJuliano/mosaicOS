# Milestone 7 — Container Prototype

## Context

MosaicOS applications do not run with direct access to system resources. Every application runs in an isolated container with explicitly declared capabilities, mount namespaces, network policies, and GUI permissions. Containers are defined declaratively and managed by the container-manager service.

This milestone produces the first working container: a native MosaicOS application running in an isolated context with controlled GUI access via the gui-proxy.

## Objective

Implement the container-manager, container manifest parser, namespace isolation via L4Re capabilities, gui-proxy for windowed GUI access from inside a container, and policy-engine for permission enforcement. Run `mosaic-hello` from Milestone 5 inside a container.

## Prerequisites

- Milestone 5 complete: compositor, display-server, and input-server work; mosaic-hello runs as a direct application
- Milestone 3 complete: service manager and capability manager work
- Milestone 6 complete: namespace manager works (containers use it for mounts)
- docs/containers.md defines the container manifest format and permission model

## Deliverables

### 1. Container manifest format v0.1

Define and document the full container manifest schema. Add to system manifest:

```yaml
containers:
  hello-app:
    runtime: native
    command: /apps/mosaic-hello
    gui:
      allow: true
      clipboard: deny
      screen_capture: deny
    mounts:
      - source: /info          # translator mount from namespace manager
        target: /sys/info
        access: ro
    network:
      mode: none
    capabilities: []
    recovery:
      max_restarts: 0          # containers: default to no restart on exit
```

Fields:
- `runtime`: `native` (this milestone), `wasm` (M8), `linux` (M9)
- `command`: path to the binary inside the container's rootfs
- `gui.allow`: whether the container may create windows
- `gui.clipboard`: `allow`, `ask`, `deny`
- `gui.screen_capture`: `allow`, `ask`, `deny`
- `mounts`: list of paths from the host namespace exposed inside the container
- `network.mode`: `none`, `isolated`, `full` (only `none` is implemented this milestone)
- `capabilities`: list of system capabilities (empty = no system access)
- `recovery`: container-specific restart policy

### 2. container-manager

Location: `containers/container-manager/`

Language: Rust

The container-manager is a service started by mosaic-init. It:
- Reads container definitions from the system manifest
- Starts containers on demand (or at boot if `autostart: true`)
- Creates an isolated task environment for each container
- Passes only declared capabilities to container tasks
- Monitors container lifecycle
- Exposes a management IPC interface

IPC interface (`sdk/ipc-api/container.rs`):
```rust
enum ContainerRequest {
    Start   { name: [u8; 64] },          // → ContainerId or Error
    Stop    { id: ContainerId },
    Kill    { id: ContainerId },
    Status  { id: ContainerId },         // → ContainerStatus
    List,                                 // → Vec<ContainerSummary>
    GetLogs { id: ContainerId, lines: u32 }, // → log lines
}

struct ContainerStatus {
    id: ContainerId,
    name: [u8; 64],
    state: ContainerState,  // Created, Starting, Running, Exited, Failed
    pid: Option<u64>,
    exit_code: Option<i32>,
    uptime_ms: u64,
}
```

### 3. Namespace isolation

Location: `containers/namespace-manager/`

Language: Rust

Implements per-container namespace isolation using L4Re capabilities:

- Each container receives a restricted capability set
- Container tasks cannot access capabilities not in their manifest
- Mount namespace: container sees only its declared `mounts`, not the full system namespace
- The namespace-manager from M6 is extended to serve per-container namespaces

Isolation implementation:
```rust
pub struct ContainerNamespace {
    mounts: HashMap<String, TranslatorCap>,  // container-local path → translator cap
    capabilities: Vec<(String, l4re::Cap)>,  // name → cap (only declared caps)
}

impl ContainerNamespace {
    pub fn build(manifest: &ContainerManifest, system_ns: &NamespaceManager) -> Result<Self>
    pub fn apply_to_task(&self, task: &mut l4re::Task) -> Result<()>
}
```

### 4. rootfs-manager

Location: `containers/rootfs-manager/`

For the native runtime, the rootfs is the host system's path namespace filtered by the container's mount declarations. There is no separate filesystem image.

The rootfs-manager:
- Constructs the container's view of the filesystem from `mounts:` declarations
- Passes translator capabilities for each declared mount to the container namespace
- For paths not in `mounts:`, access is denied

### 5. gui-proxy — windowed GUI from inside a container

Location: `containers/gui-proxy/`

Language: Rust

The gui-proxy sits between a containerized application and the compositor. It:
- Receives window creation requests from the container application
- Forwards them to the compositor on the container's behalf
- Enforces GUI permissions from the container manifest (clipboard, screen_capture)
- Labels windows with their container name (e.g., title prefix `[hello-app]`)
- Blocks clipboard access if `gui.clipboard: deny`
- Blocks screen capture requests if `gui.screen_capture: deny`

The gui-proxy is transparent to the application — the application uses the same `gui-sdk` as any direct application. The gui-proxy intercepts the IPC before it reaches the compositor.

```
[container app] → gui-sdk IPC → [gui-proxy] → compositor IPC → [compositor]
                                     ↑
                              enforces permissions
```

Each container with `gui.allow: true` gets its own gui-proxy instance.

### 6. policy-engine

Location: `containers/policy-engine/`

Language: Rust

A library (not a standalone service) used by the container-manager and gui-proxy to evaluate permission policies.

```rust
pub struct PolicyEngine {
    manifest: ContainerManifest,
}

impl PolicyEngine {
    pub fn check_gui_create_window(&self) -> PolicyDecision
    pub fn check_clipboard_read(&self) -> PolicyDecision
    pub fn check_clipboard_write(&self) -> PolicyDecision
    pub fn check_screen_capture(&self) -> PolicyDecision
    pub fn check_capability(&self, cap: &str) -> PolicyDecision
    pub fn check_mount(&self, path: &str, write: bool) -> PolicyDecision
}

enum PolicyDecision {
    Allow,
    Deny,
    Ask,   // user prompt — not implemented in M7, treated as Deny for now
}
```

### 7. mosaic-run CLI

Extend `tools/mosaicctl/` or create a new binary `tools/mosaic-run/`:

```
mosaic-run hello-app
```

Sends a `ContainerRequest::Start { name: "hello-app" }` to the container-manager and waits for the container to reach `Running` state.

```
mosaic-run list
```

Shows all containers and their states:
```
CONTAINER    STATE     RUNTIME   UPTIME
hello-app    running   native    00:00:42
```

```
mosaic-run stop hello-app
mosaic-run logs hello-app
```

### 8. Updated manifest

`boot/manifests/development.yaml` — add container-manager and hello-app container:

```yaml
services:
  container-manager:
    binary: /system/containers/container-managerd
    restart: always
    requires:
      - logger
      - namespace-manager
    capabilities:
      - process.create
      - capability.delegate

containers:
  hello-app:
    runtime: native
    command: /apps/mosaic-hello
    gui:
      allow: true
      clipboard: deny
      screen_capture: deny
    mounts:
      - source: /info
        target: /sys/info
        access: ro
    network:
      mode: none
```

## File layout

```
containers/
├── container-manager/
│   └── src/main.rs
├── namespace-manager/
│   └── src/main.rs
├── rootfs-manager/
│   └── src/main.rs
├── gui-proxy/
│   └── src/main.rs
└── policy-engine/
    └── src/lib.rs

sdk/
└── ipc-api/
    └── container.rs           (new)

tools/
├── mosaic-run/
│   └── src/main.rs
└── mosaicctl/
    └── src/
        └── containers.rs      (new: mosaicctl container subcommand)
```

## Architectural constraints

- Containers never hold capabilities that were not declared in their manifest. The container-manager must enforce this without exception.
- The gui-proxy is the only path for GUI access from a container. Containers cannot talk to the compositor directly.
- Policy decisions must be logged. Every `Deny` decision by the policy-engine must produce a log entry via logd.
- The policy-engine is a library, not a service. It runs in the container-manager and gui-proxy process space.
- Container lifecycle is managed by the container-manager, not mosaic-init directly. mosaic-init starts the container-manager; the container-manager starts containers.
- Network isolation (`mode: none`) means the container task receives no network capability whatsoever.

## Completion criteria

This milestone is complete when:

- [ ] container-manager starts and registers with mosaic-init
- [ ] `mosaic-run hello-app` starts mosaic-hello inside a container
- [ ] mosaic-hello window appears on screen (via gui-proxy → compositor)
- [ ] Window title shows the container label prefix
- [ ] Container cannot access `/info` unless declared in `mounts:`
- [ ] Clipboard access blocked when `gui.clipboard: deny` (verify via log)
- [ ] `mosaic-run list` shows hello-app in running state
- [ ] `mosaic-run stop hello-app` stops the container cleanly
- [ ] Container exit is detected and logged by container-manager
- [ ] A container attempting to use an undeclared capability receives a `Deny` result and a log entry

## Notes for the AI

- The hardest part of this milestone is L4Re capability delegation for containers. A container task must receive exactly the capabilities in its manifest and nothing more. Study L4Re's `l4re::util::Object_registry` and task capability setup carefully.
- The gui-proxy should be a thin IPC relay at this stage. It does not need to understand window content — it only filters and forwards IPC messages.
- `PolicyDecision::Ask` is declared but not implemented. When encountered, treat it as `Deny` and log a `[POLICY] ask-not-implemented, defaulting to deny` message.
- The rootfs-manager does not need to implement a full overlay filesystem. For the native runtime, it just maps translator capabilities to container-local paths. The container sees files by asking the namespace manager.
- Keep the container-manager stateless across restarts: all state is derived from the manifest and from querying running tasks. This makes recovery simpler.
- For testing isolation: after starting hello-app, try to manually send an IPC to a service the container doesn't have a capability for. Verify it gets an `InvalidCap` error from the kernel.
