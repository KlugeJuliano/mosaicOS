# Milestone 0 — Documentation and Architecture

## Context

MosaicOS Lab is a research operating system project exploring a modern graphical, microkernel-based desktop system where applications and system services are isolated by default, configured declaratively, and monitored by a native self-healing recovery layer.

This milestone produces no executable code. Its output is the foundational documentation that all subsequent milestones depend on.

## Objective

Establish the complete written foundation of the project: principles, architecture, boot flow, configuration format, and service model. Every document produced here becomes the specification that code milestones implement.

## Deliverables

### Repository structure

Create the following directory layout at the root of the repository:

```
MosaicOS/
├── LICENSE                        (MIT, already created)
├── README.md                      (project overview)
├── docs/
│   ├── architecture.md
│   ├── boot-flow.md
│   ├── graphics-stack.md
│   ├── service-model.md
│   ├── containers.md
│   ├── translators.md
│   ├── recovery.md
│   └── roadmap.md
├── microkernel/
├── boot/
├── core/
├── graphics/
├── shell/
├── services/
├── translators/
├── runtimes/
├── containers/
├── recovery/
├── sdk/
├── tools/
└── .gitignore
```

Each source directory should contain only a placeholder `README.md` at this stage.

### docs/architecture.md

Must cover:
- The 8 architectural layers (microkernel, boot/core, services, graphics, recovery, translators, runtimes/containers, shell/sdk/tools)
- Responsibility of each layer
- What each layer is NOT responsible for
- Dependency rules: which layers may communicate with which
- IPC as the only communication channel between isolated components
- ASCII or Mermaid diagram showing the layer stack

### docs/boot-flow.md

Must cover:
- Full boot sequence from bootloader to desktop shell
- Each step: what runs, what it initializes, what it hands off to next
- Role of mosaic-init as the first user-space process
- How the system manifest is loaded and parsed
- What happens if a required service fails to start during boot
- Mermaid sequence diagram of the boot flow

### docs/service-model.md

Must cover:
- Definition of a service in MosaicOS
- Service lifecycle states: defined → starting → running → failed → restarting → stopped
- How capabilities are declared and assigned
- Dependency resolution (requires: field)
- Restart policies: always, on-failure, never
- Recovery policy structure: max_restarts, fallback service
- Reference YAML schema for a service definition:

```yaml
services:
  example-service:
    binary: /system/services/exampled
    restart: on-failure
    capabilities:
      - disk.read
    requires:
      - logger
    recovery:
      max_restarts: 3
      fallback: null
```

### docs/boot-flow.md — system manifest schema

Define version 0.1 of the system manifest format. Must include:
- system block (name, profile)
- boot block (target, init binary)
- services block (list of service definitions)
- graphical block (shell binary)
- recovery block (default policy, snapshot config, per-service overrides)

Full annotated example must be included.

### docs/graphics-stack.md

Must cover:
- Why the graphical environment is a first-class boot target
- Components of the graphics stack and their responsibilities
- display-server: owns the framebuffer, talks to hardware
- compositor: owns window composition, receives buffers from apps
- window-manager: owns layout and focus policy
- input-server: receives raw input events, routes to focused window
- gui-protocol: IPC message format between apps and compositor
- What happens when the compositor crashes (recovery path)

### docs/translators.md

Must cover:
- Definition of a translator in MosaicOS (inspired by GNU Hurd)
- How translators differ from regular services
- How translators appear in the filesystem tree
- Translator lifecycle: mount → expose → unmount
- Examples: s3 translator, sqlite translator, http translator, virtual device
- Mount declaration in the system manifest:

```yaml
mounts:
  - target: /cloud
    type: translator
    translator: s3
    source: s3://my-bucket
  - target: /db/app
    type: translator
    translator: sqlite
    source: /data/app.db
```

### docs/containers.md

Must cover:
- Why applications run in containers by default
- Container manifest format
- Supported runtimes: native, wasm, linux-personality (future)
- GUI access model: gui-proxy between container and compositor
- Permission model: allow/ask/deny per resource (clipboard, screen capture, network)
- Mount namespaces inside containers
- Container lifecycle managed by the service manager

Full annotated container manifest example:

```yaml
containers:
  editor:
    runtime: native
    command: /apps/editor
    gui:
      allow: true
      clipboard: ask
      screen_capture: deny
    mounts:
      - source: /home/user/projects
        target: /workspace
        access: rw
    network:
      mode: isolated
```

### docs/recovery.md

Must cover:
- Philosophy: failure is expected, recovery is a primitive
- Components: guardian, watchdog, health-monitor, crash-manager, rollback-manager
- Heartbeat protocol between watchdog and services
- Recovery action sequence: restart → fallback → safe-gui → recovery-shell
- Safe GUI mode: minimal graphical environment for diagnostics
- Recovery shell: text-mode interface when GUI is unavailable
- Snapshot and rollback model
- Per-service recovery policy declaration

### docs/roadmap.md

Must cover:
- All 10 milestones (M0 to M9) with:
  - Objective
  - Inputs (what must exist before starting)
  - Outputs (what is produced)
  - Completion criteria (observable, testable)
- Research questions the project investigates
- Non-goals (explicit list of what this project will not do)
- Long-term directions (post M9)

## Architectural constraints

- The kernel must never parse configuration files. All manifest reading happens in user-space via mosaic-init.
- IPC is the only communication channel between isolated services. No shared memory across isolation boundaries.
- Every service must declare a recovery policy. Silent failure is not acceptable.
- Capabilities are the only mechanism for granting access to resources.

## Completion criteria

This milestone is complete when:

- [ ] Repository exists with the defined directory structure
- [ ] All 7 documentation files exist and cover the sections described above
- [ ] System manifest schema v0.1 is fully defined and annotated
- [ ] Container manifest schema v0.1 is fully defined and annotated
- [ ] Boot flow is documented step by step with a diagram
- [ ] Service lifecycle states and transitions are fully described
- [ ] Recovery action sequence is fully described
- [ ] roadmap.md covers all 10 milestones with completion criteria

## Notes for the AI

- Do not generate code in this milestone. Documentation only.
- Use Mermaid diagrams where specified. Use ASCII diagrams as fallback.
- Be precise about what each component is responsible for and what it is not.
- All configuration examples must be valid YAML.
- Write in clear technical English. Avoid vague statements like "handles various tasks".
- Every document should be self-contained: a reader of service-model.md should not need to read architecture.md to understand service lifecycles.
