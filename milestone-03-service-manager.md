# Milestone 3 — Service Manager

## Context

Milestone 2 produced a mosaic-init that can start one service and apply a basic restart policy. This milestone evolves that into a real service manager capable of starting multiple services in dependency order, tracking their lifecycle states, and exposing status via a CLI tool.

## Objective

Implement a full service manager integrated into mosaic-init that handles dependency graphs, lifecycle state machines, capability assignment, and exposes service status via `mosaicctl status`.

## Prerequisites

- Milestone 2 complete: mosaic-init starts logd and echod, IPC logging works
- `sdk/ipc-api/log.rs` exists and is usable by services
- System manifest parser handles `requires:` and `capabilities:` fields

## Deliverables

### 1. Dependency resolver

Location: `core/init/src/resolver.rs`

Input: the full services map from the manifest
Output: a topologically sorted list of services in start order

Requirements:
- Detect circular dependencies and abort with a clear error
- Services with no `requires:` field start first (in declaration order)
- A service only starts after all its `requires:` services are in `running` state
- If a required service fails to start, dependent services are not started and are marked `blocked`

Example dependency graph from the manifest:
```
logger       (no deps)
storage      requires: logger
filesystem   requires: storage
display      requires: device-manager
compositor   requires: display, input
```

Correct start order: logger → storage → filesystem → device-manager → display + input (parallel) → compositor

### 2. Service lifecycle state machine

Location: `core/init/src/lifecycle.rs`

States:
```
defined → starting → running → failed → restarting → stopped
                  ↘ blocked (if a dependency failed)
```

Transitions:
- `defined → starting`: mosaic-init begins spawning the task
- `starting → running`: task sends its first heartbeat (or first log message as proxy for now)
- `running → failed`: task exits unexpectedly or heartbeat times out (heartbeat in M4)
- `failed → restarting`: restart policy allows restart, delay applied
- `restarting → starting`: restart attempt begins
- `failed → stopped`: max_restarts exceeded or policy is `never`
- `starting → failed`: task exits before reaching `running` state

Each transition must be logged via logd.

### 3. Capability manager

Location: `core/init/src/capabilities.rs`

Expand the simple capability mapping from Milestone 2 into a proper capability table:

```rust
pub struct CapabilityTable {
    entries: HashMap<String, l4re::Cap>,
}

impl CapabilityTable {
    pub fn resolve(&self, name: &str) -> Result<l4re::Cap, CapError>
    pub fn grant(&self, name: &str, task: &l4re::Task) -> Result<(), CapError>
    pub fn audit(&self) -> Vec<CapAuditEntry>  // lists all caps and which services hold them
}
```

The `audit()` method is used by `mosaicctl caps` to inspect what capabilities are active.

### 4. Service registry

Location: `core/init/src/registry.rs`

A runtime registry that tracks all services and their current state. Other components (watchdog in M4, mosaicctl) query this registry via IPC.

```rust
pub struct ServiceRegistry {
    services: HashMap<String, ServiceEntry>,
}

pub struct ServiceEntry {
    pub name: String,
    pub state: ServiceState,
    pub pid: Option<l4re::TaskCap>,
    pub restart_count: u32,
    pub last_state_change: u64,  // timestamp ms
}
```

IPC interface (define in `sdk/ipc-api/registry.rs`):
- `ListServices() → Vec<ServiceSummary>`
- `GetService(name: &str) → Option<ServiceEntry>`
- `GetState(name: &str) → Option<ServiceState>`

### 5. mosaicctl — CLI tool

Location: `tools/mosaicctl/`

Language: Rust

A command-line tool that queries mosaic-init via IPC and displays service status.

Commands for this milestone:

```
mosaicctl status
```

Output format:
```
SERVICE          STATE       RESTARTS   UPTIME
logger           running     0          00:03:42
storage          running     0          00:03:40
filesystem       running     1          00:02:15
compositor       failed      3          —
network          blocked     —          —
```

```
mosaicctl status <service-name>
```

Detailed output for one service including state history and capabilities held.

```
mosaicctl caps
```

Lists all capabilities and which services currently hold them.

mosaicctl communicates with mosaic-init via IPC using the service registry interface defined above.

### 6. Updated system manifest

Expand `boot/manifests/development.yaml` to include 4+ services with real dependencies:

```yaml
system:
  name: MosaicOS
  profile: development

boot:
  target: minimal
  init: /system/core/mosaic-init

services:
  logger:
    binary: /system/services/logd
    restart: always
    capabilities:
      - serial.out

  storage:
    binary: /system/services/storaged
    restart: always
    requires:
      - logger
    capabilities:
      - disk.read
      - disk.write

  filesystem:
    binary: /system/services/vfsd
    restart: on-failure
    requires:
      - storage

  device-manager:
    binary: /system/services/devd
    restart: always
    requires:
      - logger

  echo:
    binary: /system/services/echod
    restart: never
    requires:
      - logger
      - filesystem
```

### 7. Stub services

For testing the dependency chain, create minimal stub implementations for:
- `services/storage/storaged/` — logs "storaged: ready", stays running
- `services/filesystem/vfsd/` — logs "vfsd: ready", stays running
- `services/device-manager/devd/` — logs "devd: ready", stays running

Each stub must:
- Accept the IPC port from mosaic-init
- Send a "ready" log message
- Enter an IPC wait loop (simulating a running service)
- Exit cleanly when sent a shutdown message

## File layout

```
core/
└── init/
    └── src/
        ├── main.rs
        ├── manifest.rs
        ├── resolver.rs        (new)
        ├── lifecycle.rs       (new)
        ├── capabilities.rs    (expanded)
        ├── registry.rs        (new)
        └── health.rs

tools/
└── mosaicctl/
    └── src/
        ├── main.rs
        ├── status.rs
        └── caps.rs

sdk/
└── ipc-api/
    ├── log.rs
    └── registry.rs            (new)

services/
├── logging/logd/
├── echo/echod/
├── storage/storaged/          (stub)
├── filesystem/vfsd/           (stub)
└── device-manager/devd/       (stub)
```

## Architectural constraints

- Dependency resolution happens at boot time, before any service is started. If the graph is invalid, mosaic-init aborts with a clear error.
- Services communicate with the service manager only via IPC. No shared state.
- mosaicctl is a separate task. It does not link against mosaic-init internals. It uses only the IPC registry interface.
- The capability manager must never grant a capability that was not declared in the manifest.
- All state transitions must be logged with timestamps.

## Completion criteria

This milestone is complete when:

- [ ] mosaic-init starts 4+ services in correct dependency order
- [ ] Circular dependency in manifest produces a clear error and aborts boot
- [ ] A failed dependency correctly blocks dependent services (state: `blocked`)
- [ ] `mosaicctl status` shows all services with correct states and restart counts
- [ ] `mosaicctl caps` shows all capabilities and their holders
- [ ] Killing a stub service (simulating crash) triggers restart policy correctly
- [ ] `restart: always` service restarts, `restart: never` service transitions to `stopped`
- [ ] All state transitions are logged via logd with timestamps
- [ ] All 4+ stub services start and reach `running` state

## Notes for the AI

- Use Kahn's algorithm for topological sort. It handles cycle detection naturally.
- The service registry should be the single source of truth for service state. mosaic-init internal components query the registry, not each other directly.
- mosaicctl can be a simple synchronous IPC client at this stage. No need for async.
- Stub services that stay running should use `l4_ipc_wait()` in a loop. This is more realistic than `sleep()` and exercises the IPC path.
- For simulating a crash in tests, simply kill the task from a test harness or add a `--crash-after=N` flag to a stub service.
- Keep the IPC message types for the registry simple: fixed-size structs, no heap allocation on the IPC path.
