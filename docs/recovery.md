# MosaicOS Recovery Layer

Failure is expected; recovery is a primitive. MosaicOS is built on the principle that any component can fail, and the system must be able to heal itself without a full reboot.

## Core Philosophy
- **Observability:** You cannot recover what you cannot monitor.
- **Isolation:** Failure in one service should not cascade.
- **Predictability:** Recovery actions follow a strictly defined sequence.

## Components

### 1. The Guardian (`guardiand`)
- The central orchestrator of the recovery layer.
- Consults the system manifest for recovery policies.
- Instructs `mosaic-init` to perform restarts or fallbacks.

### 2. Watchdog
- A per-service monitoring mechanism.
- Expects a periodic "heartbeat" from the service.
- If the heartbeat is missed or the process exits, it notifies the Guardian.

### 3. Health Monitor
- Monitors system-wide metrics (memory, CPU, IPC latency).
- Detects hangs or "zombie" states where a process is technically running but unresponsive.

### 4. Crash Manager
- Collects core dumps, logs, and diagnostic data when a service fails.
- Provides reports for later analysis.

## Recovery Action Sequence

When a service failure is detected, the Guardian follows this sequence:

1. **Restart:** Attempt to relaunch the service (up to `max_restarts`).
2. **Fallback:** If restarts fail, switch to a simpler `fallback` implementation defined in the manifest.
3. **Safe GUI Mode:** If a critical graphical component (like the compositor) fails persistently, boot into a minimal, high-reliability graphical environment.
4. **Recovery Shell:** If the graphical environment is completely lost, provide a text-based emergency shell for manual diagnostics.
5. **Rollback:** Revert to a previous known-good system snapshot.

## Current Prototype

The current lab prototype implements the first restart path inside `mosaic-init`.
For services that declare `restart: on-failure` or a non-zero
`recovery.max_restarts`, `mosaic-init` starts the task through Ned, waits for its
exit result, and restarts it until the configured retry budget is exhausted.

The `mosaicos-recovery` lab entry demonstrates this with `mosaic-crash`, a
controlled service that exits with failure. The expected behavior is two restarts
and a final `failed` state after the third failed attempt.

This is a recovery prototype, not the final Guardian/watchdog architecture. The
next step is replacing synchronous exit monitoring with asynchronous heartbeats
and process-exit notifications for long-running services.

## Snapshot & Rollback
MosaicOS supports atomic system snapshots. Before updates or major configuration changes, a snapshot is taken. If the system fails to reach a stable state, it can automatically rollback to the last successful boot configuration.

## Service Policy Example

```yaml
services:
  compositor:
    recovery:
      max_restarts: 3
      fallback: safe-gui
      on_critical_failure: notify_user
```
