# MosaicOS Containers

In MosaicOS, applications do not run directly on the host system. They are executed within isolated environments called **Containers**.

## Why Containers?
Isolation by default ensures that an application cannot access files, hardware, or other processes unless explicitly permitted. This minimizes the impact of security vulnerabilities and application crashes.

## Container Runtimes

1. **Native Runtime:** Optimized for MosaicOS-native binaries using the system's IPC and GUI protocols.
2. **WASM Runtime:** Executes WebAssembly modules in a sandboxed environment, providing cross-platform compatibility and extreme safety.
3. **Linux Personality (Future):** A compatibility layer to run unmodified Linux binaries.

## GUI Access Model
Containers interact with the graphics stack through a **GUI Proxy**.
- The proxy manages the container's connection to the `compositord`.
- It enforces permission rules (e.g., "Can this app take a screenshot?").
- It provides a virtualized input stream.

## Permissions & Capabilities
Permissions are granular and can be configured as `allow`, `ask`, or `deny`.
- **Clipboard:** Access to copy/paste buffers.
- **Screen Capture:** Ability to read the contents of other windows.
- **Network:** Access to external networking.
- **Devices:** Access to specific hardware (webcam, microphone).

## Container Manifest (v0.1)

```yaml
containers:
  editor:
    runtime: native
    command: /apps/editor
    gui:
      allow: true
      clipboard: ask        # User will be prompted
      screen_capture: deny
    mounts:
      - source: /home/user/projects
        target: /workspace
        access: rw
    network:
      mode: isolated        # No external network access
    capabilities:
      - font.read
```

## Lifecycle Management
The `container-manager` (a core service) is responsible for:
- Starting and stopping containers.
- Managing mount namespaces.
- Monitoring resource usage.
- Enforcing security policies via the microkernel's capability system.
