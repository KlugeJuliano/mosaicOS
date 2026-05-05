# MosaicOS Roadmap

MosaicOS is developed through a series of iterative milestones, moving from theoretical architecture to a functional, graphical, self-healing prototype.

## Milestones

### Milestone 0 — Documentation and Architecture
- **Objective:** Establish the complete written foundation.
- **Inputs:** Vision and project goals.
- **Outputs:** Repository structure, core documentation files (`docs/`).
- **Criteria:** All documentation specified in `milestone-00-documentation.md` exists and is consistent.

### Milestone 1 — L4Re Laboratory
- **Objective:** Establish a working build and boot environment.
- **Inputs:** Milestone 0 documentation.
- **Outputs:** Built L4Re kernel, QEMU runner scripts, Hello World task.
- **Criteria:** Kernel boots in QEMU and executes a user-space task.

### Milestone 2 — Minimal Init
- **Objective:** Create the first service orchestrator.
- **Inputs:** Milestone 1 environment.
- **Outputs:** `mosaic-init` binary, simple service manifest parser.
- **Criteria:** `mosaic-init` starts a single service from a configuration file.

### Milestone 3 — Service Manager
- **Objective:** Orchestrate multiple services with dependencies.
- **Inputs:** Milestone 2 `mosaic-init`.
- **Outputs:** Parallel service startup, status tracking, dependency resolution.
- **Criteria:** Services start in the correct order and their status is observable.

### Milestone 4 — Recovery Prototype
- **Objective:** Implement basic self-healing.
- **Inputs:** Milestone 3 Service Manager.
- **Outputs:** Guardian service, watchdog mechanism, restart logic.
- **Criteria:** A crashed service is automatically restarted by the Guardian.

### Milestone 5 — Graphical Prototype
- **Objective:** Boot into a graphical environment.
- **Inputs:** Milestone 4 system.
- **Outputs:** Framebuffer display server, minimal compositor, input handler.
- **Criteria:** System boots to a screen with a rendered window and mouse cursor.

### Milestone 6 — Translator Prototype
- **Objective:** Explore Hurd-inspired filesystem abstraction.
- **Inputs:** Milestone 5 system.
- **Outputs:** A virtual filesystem translator (e.g., exposing a static resource).
- **Criteria:** Files exposed by the translator are accessible via standard file operations.

### Milestone 7 — Container Prototype
- **Objective:** Secure isolated application execution.
- **Inputs:** Milestone 6 system.
- **Outputs:** Container manifest format, native application sandbox, GUI proxy.
- **Criteria:** Application runs in a restricted environment with mediated GUI access.

### Milestone 8 — WASM Runtime Experiment
- **Objective:** Test WebAssembly as a safe application format.
- **Inputs:** Milestone 7 system.
- **Outputs:** WASM host environment, controlled system APIs.
- **Criteria:** A WASM module executes and interacts with MosaicOS services.

### Milestone 9 — Linux Personality Research
- **Objective:** Investigate ABI compatibility.
- **Inputs:** Milestone 8 system.
- **Outputs:** ELF loader, subset of Linux syscall implementation.
- **Criteria:** A minimal Linux binary (static `write`) runs on MosaicOS.

## Research Questions
- Can a graphical OS be designed around isolation from the beginning?
- Can declarative configuration simplify system composition and recovery?
- Can Hurd-like translators be modernized for cloud and APIs?
- Can automatic recovery be treated as a core OS feature?

## Non-Goals
- Replacing Linux or mainstream production OSs.
- Full POSIX compatibility in the early stages.
- Supporting high-end GPU acceleration or complex games.
- Production-grade security in the prototype phase.
