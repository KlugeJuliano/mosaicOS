# MosaicOS Service Model

In MosaicOS, a service is an isolated, user-space process that provides specific functionality to other components via IPC.

## Service Lifecycle

Services progress through the following states:

1. **Defined:** The service is described in the manifest but not yet started.
2. **Starting:** `mosaic-init` has spawned the process and is waiting for a "ready" signal.
3. **Running:** The service is active and responding to IPC requests.
4. **Failed:** The service process has exited unexpectedly or failed a health check.
5. **Restarting:** The recovery layer is attempting to relaunch the service.
6. **Stopped:** The service has been gracefully shut down.

## Service Definition Schema

Services are defined using a YAML-based manifest.

```yaml
services:
  example-service:
    binary: /system/services/exampled
    restart: on-failure  # always, on-failure, never
    capabilities:
      - disk.read        # Specific permission grant
    requires:
      - logger           # Dependency ordering
    recovery:
      max_restarts: 3
      fallback: null     # Optional service to run if this one fails permanently
```

### Capabilities
Capabilities are the only mechanism for granting access to resources. A service must explicitly declare the capabilities it requires. Examples include:
- `disk.read` / `disk.write`
- `net.client` / `net.server`
- `gpu.render`
- `hw.input`

### Dependency Resolution
The `requires` field ensures that services are started in the correct order. `mosaic-init` builds a dependency graph and starts services in parallel when possible, respecting these constraints.

### Restart Policies
- **always:** Restart the service regardless of why it exited.
- **on-failure:** Restart only if the exit code is non-zero.
- **never:** Do not restart.

### Recovery Policy
The `recovery` block defines how the system should handle persistent failures of a specific service. If `max_restarts` is reached, the system may switch to a `fallback` service or trigger a broader system recovery action.

The current lab prototype supports `recovery.max_restarts` for services launched
under exit monitoring. A service with `restart: on-failure` is relaunched until
the retry budget is exhausted, then marked `failed`.
