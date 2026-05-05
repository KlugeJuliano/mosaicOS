# Milestone 2 — Minimal mosaic-init

## Context

MosaicOS Lab uses a user-space init process called `mosaic-init` as the first meaningful process after the kernel boots. The kernel itself never parses configuration files — that responsibility belongs entirely to mosaic-init.

This milestone produces the first version of mosaic-init: a process that reads a minimal system manifest, starts one user-space service, and maintains a basic logging channel.

## Objective

Implement `mosaic-init` v0.1 that can read a YAML/TOML system manifest, spawn a single user-space service as an L4Re task, and route log messages from that service to a log daemon (`logd`).

## Prerequisites

- Milestone 1 complete: L4Re builds and runs in QEMU, IPC between tasks is validated
- docs/service-model.md defines the manifest schema and service lifecycle
- docs/boot-flow.md defines the role of mosaic-init in the boot sequence

## Deliverables

### 1. mosaic-init binary

Location: `core/init/`

Language: Rust (preferred) or C

Responsibilities:
- Be the first user-space process started by the L4Re bootstrap
- Read and parse the system manifest from a known location
- Validate the manifest (required fields, unknown fields, type errors)
- Spawn each declared service as an L4Re task
- Pass declared capabilities to each spawned task
- Monitor spawned tasks at a basic level (detect exit)
- Restart a service if its restart policy is `always` or `on-failure`

mosaic-init must NOT:
- Parse complex dependency graphs (that is Milestone 3)
- Implement watchdog heartbeats (that is Milestone 4)
- Start more than one service in this milestone (keep scope minimal)

### 2. logd — log daemon

Location: `services/logging/logd/`

Language: Rust or C

Responsibilities:
- Receive log messages from other services via IPC
- Write messages to serial output with timestamp and service name prefix
- Stay running permanently (restart: always)

Log message IPC format (define in `sdk/ipc-api/log.h` or `log.rs`):
```
struct LogMessage {
    u64  timestamp_ms;
    u8   level;         // 0=debug, 1=info, 2=warn, 3=error
    char service[32];
    char message[256];
}
```

### 3. System manifest v0.1 parser

Location: `core/init/manifest.rs` or `core/init/manifest.c`

Must parse this minimal manifest format:

```yaml
system:
  name: MosaicOS
  profile: development

boot:
  target: graphical
  init: /system/core/mosaic-init

services:
  logger:
    binary: /system/services/logd
    restart: always
```

Validation rules:
- `system.name` is required
- `boot.init` is required
- Each service must have a `binary` field
- `restart` must be one of: `always`, `on-failure`, `never`
- Unknown top-level keys produce a warning, not an error
- Missing optional fields use defaults (restart defaults to `on-failure`)

### 4. Capability passing

mosaic-init must read the `capabilities` list from the manifest and pass those capabilities to the spawned task via L4Re capability delegation.

Example manifest with capabilities:
```yaml
services:
  storage:
    binary: /system/services/storaged
    restart: always
    capabilities:
      - disk.read
      - disk.write
```

For this milestone, capabilities are symbolic names. The actual capability objects are pre-allocated and mapped by name in mosaic-init's capability table. A simple hardcoded mapping is acceptable:

```rust
fn resolve_capability(name: &str) -> Option<l4re::Cap> {
    match name {
        "disk.read"  => Some(CAP_DISK_READ),
        "disk.write" => Some(CAP_DISK_WRITE),
        "serial.out" => Some(CAP_SERIAL),
        _ => None,
    }
}
```

### 5. Basic health check

mosaic-init must detect when a service task exits unexpectedly and apply the restart policy:

- `restart: always` — restart immediately, log the restart event
- `restart: on-failure` — restart only if exit code is non-zero, log the event
- `restart: never` — log the exit and do not restart

For this milestone, detection is passive (waiting on task exit via L4Re wait). Active heartbeat is Milestone 4.

### 6. Example manifest

Location: `boot/manifests/development.yaml`

A working manifest that boots logd and one example service (can be a trivial echo service that logs "hello" every second and exits):

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

  echo:
    binary: /system/services/echod
    restart: on-failure
    requires:
      - logger
```

### 7. echod — example service

Location: `services/echo/echod/`

A trivial service that:
- Sends 3 log messages via IPC to logd
- Exits cleanly with code 0

This service exists only to validate that mosaic-init can spawn a service, that the service can log, and that mosaic-init detects the exit correctly.

## IPC design for logging

All services log by sending IPC messages to logd. mosaic-init passes the logd capability to every service it spawns.

```
[service] --IPC LogMessage--> [logd] --serial write--> /dev/serial
```

Define the log IPC interface in `sdk/ipc-api/`. This interface must be usable by all future services.

## File layout

```
core/
└── init/
    ├── src/
    │   ├── main.rs
    │   ├── manifest.rs
    │   ├── spawner.rs
    │   └── health.rs
    ├── Cargo.toml (or Makefile)
    └── README.md

services/
├── logging/
│   └── logd/
│       ├── src/main.rs
│       └── Cargo.toml
└── echo/
    └── echod/
        ├── src/main.rs
        └── Cargo.toml

sdk/
└── ipc-api/
    └── log.rs (or log.h)

boot/
└── manifests/
    └── development.yaml
```

## Architectural constraints

- mosaic-init is not a general-purpose init system. It manages MosaicOS services only.
- The kernel never sees the manifest. mosaic-init owns all manifest parsing.
- IPC is the only communication channel between mosaic-init, logd, and services.
- Capabilities are the only access control mechanism. No ambient authority.
- mosaic-init must log its own actions through logd once logd is running. Before logd starts, it may write directly to serial as a fallback.

## Completion criteria

This milestone is complete when:

- [ ] mosaic-init parses `development.yaml` without errors
- [ ] mosaic-init spawns logd as an L4Re task
- [ ] mosaic-init spawns echod as an L4Re task after logd is running
- [ ] echod sends log messages via IPC to logd
- [ ] logd writes formatted messages to serial: `[INFO] [echo] hello from echod`
- [ ] echod exits, mosaic-init detects the exit and logs the event
- [ ] restart policy `on-failure` is correctly applied (echod exits 0, so it is not restarted)
- [ ] An invalid manifest (missing `binary`) produces a clear error message on serial
- [ ] All code builds reproducibly using the scripts from Milestone 1

## Notes for the AI

- Use a minimal YAML parser. Do not pull in a heavy dependency. For Rust, `serde_yaml` is acceptable. For C, a minimal hand-rolled parser or `libyaml` is fine.
- L4Re task spawning uses `l4re::util::Object_registry` or the `Ned` script server depending on the L4Re version. Check which mechanism is available in the laboratory environment from Milestone 1.
- mosaic-init must handle the case where a service binary does not exist. Log a clear error and continue with the remaining services.
- Keep the IPC message format simple and fixed-size at this stage. Variable-length messages add complexity that is not needed yet.
- The echo service is intentionally trivial. Resist the temptation to add features to it.
- Write tests for the manifest parser if the toolchain supports it. At minimum, test: valid manifest, missing required field, unknown restart policy, empty services block.
