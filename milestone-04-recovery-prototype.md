# Milestone 4 — Recovery Prototype

## Context

MosaicOS treats failure as an expected condition and recovery as a first-class OS primitive. Milestone 3 introduced passive failure detection (detecting task exit). This milestone adds active monitoring via a watchdog heartbeat, a full recovery action pipeline, crash logging, and a recovery shell for when all else fails.

## Objective

Implement the guardian, watchdog, and crash-manager components. Introduce heartbeat-based health monitoring, the full recovery action sequence (restart → fallback → safe-mode → recovery-shell), structured crash logs, and a minimal recovery shell.

## Prerequisites

- Milestone 3 complete: service manager starts multiple services in dependency order, lifecycle state machine works, mosaicctl status works
- `sdk/ipc-api/registry.rs` exists with `ListServices` and `GetState` interfaces
- All stub services are running and stable

## Deliverables

### 1. Watchdog heartbeat protocol

Location: `sdk/ipc-api/watchdog.rs`

Define the heartbeat IPC interface:

```rust
// Sent by each service to the watchdog on a regular interval
struct Heartbeat {
    service_name: [u8; 32],
    timestamp_ms: u64,
    status: HeartbeatStatus,  // Healthy, Degraded, Overloaded
}

// Sent by watchdog to a service to request a heartbeat
struct HeartbeatRequest {
    deadline_ms: u64,  // service must respond before this timestamp
}

// Watchdog registration (service calls this at startup)
struct RegisterRequest {
    service_name: [u8; 32],
    interval_ms: u32,     // expected heartbeat interval
    timeout_ms: u32,      // time after which missed heartbeat = failure
}
```

Services that want active monitoring must:
1. Call `watchdog.register()` at startup
2. Send a `Heartbeat` message every `interval_ms`

Services that do not register are monitored passively only (exit detection).

### 2. guardian — the recovery orchestrator

Location: `recovery/guardian/`

Language: Rust

The guardian is the top-level recovery component. It:
- Subscribes to failure events from the watchdog and crash-manager
- Decides what recovery action to take based on the service's recovery policy
- Executes the recovery action sequence
- Logs all recovery events to logd

Recovery action sequence (in order):
1. **Restart** — restart the failed service (up to `max_restarts` times)
2. **Fallback** — start the fallback service declared in the manifest
3. **Safe GUI** — if the failed service is graphical, switch to safe-gui mode
4. **Recovery shell** — last resort: start the recovery-shell service

The guardian reads recovery policies from the system manifest via mosaic-init's registry IPC.

Recovery policy fields (expand manifest schema):
```yaml
services:
  compositor:
    binary: /system/graphics/compositord
    restart: on-failure
    recovery:
      max_restarts: 3
      fallback: safe-gui
      timeout_between_restarts_ms: 1000

  display:
    binary: /system/graphics/displayd
    restart: on-failure
    recovery:
      max_restarts: 2
      fallback: framebuffer-display

  network:
    binary: /system/services/networkd
    restart: on-failure
    recovery:
      max_restarts: 5
      fallback: offline-mode
```

### 3. watchdog — active health monitor

Location: `recovery/watchdog/`

Language: Rust

The watchdog:
- Maintains a timer for each registered service
- Sends `HeartbeatRequest` to each service before its deadline
- If no `Heartbeat` is received within `timeout_ms`, emits a `FailureEvent` to the guardian
- Distinguishes between: `MissedHeartbeat`, `DegradedHeartbeat`, `TaskExit`

```rust
enum FailureEvent {
    MissedHeartbeat { service: String, last_seen_ms: u64 },
    DegradedHeartbeat { service: String, status: HeartbeatStatus },
    TaskExit { service: String, exit_code: i32 },
}
```

### 4. crash-manager — crash log collection

Location: `recovery/crash-manager/`

Language: Rust

On receiving a `TaskExit` or `MissedHeartbeat` event, the crash-manager:
- Records a structured crash log entry
- Captures: timestamp, service name, failure type, exit code (if available), restart count, recovery action taken
- Writes crash logs to a persistent location (for this milestone: serial output with a distinct `[CRASH]` prefix)

Crash log format:
```
[CRASH] 2026-05-04T14:32:01Z service=compositor reason=MissedHeartbeat restart_count=2 action=restart
[CRASH] 2026-05-04T14:32:04Z service=compositor reason=TaskExit exit_code=139 restart_count=3 action=fallback:safe-gui
```

### 5. health-monitor — periodic status reporter

Location: `recovery/health-monitor/`

A lightweight service that:
- Polls the service registry every 5 seconds
- Logs a summary of service states to logd
- Emits a warning if any service has restarted more than once in the last minute

Output example:
```
[HEALTH] 2026-05-04T14:32:00Z all_services=5 running=4 failed=0 restarted_recently=1
[WARN]   2026-05-04T14:32:00Z service=compositor restarted 2 times in last 60s
```

### 6. Recovery shell

Location: `recovery/recovery-shell/`

Language: Rust

A minimal text-mode shell activated when a critical service cannot be recovered. It must:
- Accept input from serial (keyboard)
- Provide these commands:
  - `status` — show all service states
  - `restart <service>` — manually restart a service
  - `stop <service>` — stop a service
  - `logs` — show last 20 crash log entries
  - `reboot` — trigger system reboot
  - `help` — show available commands
- Communicate with the service registry and guardian via IPC

The recovery shell does not depend on the graphical stack. It works over serial only.

### 7. Crash simulation test

Location: `tools/lab/crash-sim.sh`

A shell script that:
1. Boots the full manifest in QEMU
2. Waits for all services to reach `running` state
3. Kills one service (simulating a crash)
4. Observes and records recovery events on serial output
5. Verifies the correct recovery action was taken

The script must exit 0 if recovery succeeds within a timeout, non-zero otherwise.

### 8. Updated system manifest

`boot/manifests/development.yaml` — add recovery policies and watchdog registration to all services:

```yaml
recovery:
  default_policy: restart_then_fallback

  services:
    compositor:
      max_restarts: 3
      fallback: safe-gui

    display:
      max_restarts: 2
      fallback: framebuffer-display

    network:
      max_restarts: 5
      fallback: offline-mode
```

## File layout

```
recovery/
├── guardian/
│   └── src/main.rs
├── watchdog/
│   └── src/main.rs
├── crash-manager/
│   └── src/main.rs
├── health-monitor/
│   └── src/main.rs
└── recovery-shell/
    └── src/main.rs

sdk/
└── ipc-api/
    ├── log.rs
    ├── registry.rs
    └── watchdog.rs             (new)

tools/
└── lab/
    └── crash-sim.sh            (new)
```

## Architectural constraints

- The guardian is the single decision-maker for recovery actions. The watchdog only detects and reports. The crash-manager only records.
- Recovery actions must be logged before they are executed.
- A recovery shell must never depend on any service it might need to restart. It is completely self-contained.
- The watchdog must be resilient to its own failures. If the watchdog crashes, mosaic-init must restart it immediately (highest priority restart).
- Crash logs must be written to serial even if logd is down. The crash-manager has a direct serial capability as fallback.

## Completion criteria

This milestone is complete when:

- [ ] Services with heartbeat registration send heartbeats every `interval_ms`
- [ ] Watchdog detects a missed heartbeat and emits a `FailureEvent`
- [ ] Guardian receives `FailureEvent` and executes restart
- [ ] After `max_restarts` exceeded, guardian starts the fallback service
- [ ] Crash-manager logs all failure events with the `[CRASH]` prefix to serial
- [ ] health-monitor prints a summary every 5 seconds
- [ ] Killing a service manually triggers the full recovery sequence correctly
- [ ] Recovery shell starts when recovery is exhausted, accepts `status` and `restart` commands
- [ ] `crash-sim.sh` runs end-to-end and exits 0
- [ ] mosaicctl status shows updated restart counts and states in real time

## Notes for the AI

- The watchdog timer must be implemented carefully to avoid false positives. Use a generous default timeout (e.g., 5× the heartbeat interval) during initial testing.
- The guardian should serialize recovery decisions. Two simultaneous failures should be handled sequentially, not concurrently, to avoid cascade recovery issues.
- For the recovery shell serial input, use L4Re's serial input capability. Input is line-buffered at the QEMU level.
- The `fallback` service is a completely separate binary declared in the manifest. For the stub fallback, create a `safe-gui` stub that logs "safe-gui: active" and stays running.
- Implement a `--inject-failure` flag on stub services for testing. This makes crash simulation deterministic.
- Keep the recovery shell commands minimal. Each command is a single IPC call to the registry or guardian. No complex parsing needed.
