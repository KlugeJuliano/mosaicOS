# MosaicOS Lab

**MosaicOS Lab** is an experimental operating system research project exploring the design of a modern graphical, microkernel-based desktop system where applications and system services are isolated by default, configured declaratively, and monitored by a native self-healing recovery layer.

This project is not intended to compete with Linux, FreeBSD, macOS, or Windows. Instead, it is a long-term research and learning effort focused on understanding how operating systems could be designed if isolation, recoverability, graphical environments, service composition, and declarative configuration were first-class architectural principles from the beginning.

## Vision

Modern operating systems evolved through decades of practical engineering, compatibility requirements, hardware constraints, and legacy design decisions. MosaicOS Lab asks a different question:

> What would a desktop operating system look like if it were designed today around microkernel principles, container-native execution, declarative service orchestration, graphical-first interaction, and automatic recovery?

The goal is to investigate this idea through documents, experiments, prototypes, and eventually a minimal running system.

## Core Ideas

MosaicOS Lab is based on the following principles:

- **Microkernel-first**  
  The kernel should remain small and focused on low-level mechanisms such as memory management, scheduling, IPC, capabilities, and hardware isolation.

- **Graphical by default**  
  The system is designed from the beginning to boot into a graphical environment, not as a terminal-first system with a desktop added later.

- **Container-native execution**  
  Applications should run inside isolated environments by default, inspired by Docker, FreeBSD Jails, and modern sandboxing models.

- **User-space system services**  
  Core components such as storage, networking, filesystems, drivers, logging, graphics, and audio should run as isolated services outside the kernel whenever possible.

- **Declarative system configuration**  
  System services, containers, mounts, graphical sessions, recovery policies, and permissions should be described using configuration files similar in spirit to `docker-compose.yml`.

- **Hurd-inspired translators**  
  The system should explore the idea of translators: user-space services that expose filesystems, APIs, databases, devices, remote storage, or virtual resources as part of the filesystem tree.

- **Self-healing architecture**  
  System services should be observable, restartable, replaceable, and recoverable. A crash in the compositor, network stack, or driver service should not necessarily bring the whole system down.

- **Capability-based security**  
  Services and applications should receive only the permissions they explicitly need.

## Conceptual Architecture

```txt
MosaicOS
├── microkernel/
│   └── L4Re / Fiasco.OC or another L4-based foundation
│
├── boot/
│   ├── bootloader
│   ├── initial task
│   ├── root server
│   └── system manifest loader
│
├── core/
│   ├── init/
│   ├── service-manager/
│   ├── capability-manager/
│   ├── process-manager/
│   ├── memory-manager/
│   └── ipc-router/
│
├── graphics/
│   ├── display-server/
│   ├── compositor/
│   ├── window-manager/
│   ├── input-server/
│   ├── font-server/
│   ├── theme-server/
│   ├── clipboard-server/
│   ├── notification-server/
│   └── gui-protocol/
│
├── shell/
│   ├── desktop-shell/
│   ├── panel/
│   ├── launcher/
│   ├── settings/
│   ├── file-manager/
│   └── terminal/
│
├── services/
│   ├── storage/
│   ├── filesystem/
│   ├── network/
│   ├── device-manager/
│   ├── driver-host/
│   ├── auth/
│   ├── logging/
│   ├── audio/
│   └── power/
│
├── translators/
│   ├── file-translators/
│   ├── network-translators/
│   ├── cloud-translators/
│   ├── database-translators/
│   └── virtual-device-translators/
│
├── runtimes/
│   ├── native-runtime/
│   ├── linux-personality/
│   ├── bsd-personality/
│   └── wasm-runtime/
│
├── containers/
│   ├── container-manager/
│   ├── namespace-manager/
│   ├── jail-manager/
│   ├── rootfs-manager/
│   ├── gui-proxy/
│   └── policy-engine/
│
├── recovery/
│   ├── guardian/
│   ├── watchdog/
│   ├── health-monitor/
│   ├── crash-manager/
│   ├── rollback-manager/
│   ├── snapshot-manager/
│   ├── safe-gui/
│   ├── recovery-shell/
│   └── diagnostics/
│
├── sdk/
│   ├── libc-port/
│   ├── rust-std-port/
│   ├── syscall-api/
│   ├── ipc-api/
│   ├── gui-sdk/
│   └── translator-sdk/
│
├── tools/
│   ├── mosaicctl
│   ├── mosaic-compose
│   ├── mosaic-run
│   ├── mosaic-build
│   └── mosaic-debug
│
└── docs/
    ├── architecture.md
    ├── app-model.md
    ├── boot-flow.md
    ├── graphics-stack.md
    ├── service-model.md
    ├── containers.md
    ├── translators.md
    ├── recovery.md
    └── roadmap.md
Boot Flow

The intended boot flow is:

Bootloader
    ↓
Microkernel
    ↓
Initial task / root server
    ↓
mosaic-init
    ↓
Core services
    ↓
Display server
    ↓
Compositor
    ↓
Desktop shell
    ↓
Applications and containers

The kernel itself should not parse complex system configuration files. Instead, an initial user-space service, tentatively called mosaic-init, is responsible for reading the system manifest and starting the required services.

Declarative System Configuration

A MosaicOS system could eventually be described through configuration files such as:

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

  storage:
    binary: /system/services/storaged
    restart: always
    capabilities:
      - disk.read
      - disk.write

  filesystem:
    binary: /system/services/vfsd
    requires:
      - storage

  display:
    binary: /system/graphics/displayd
    requires:
      - device-manager

  compositor:
    binary: /system/graphics/compositord
    requires:
      - display
      - input
    recovery:
      strategy: restart
      fallback: safe-gui

graphical:
  shell: /system/shell/desktop-shell

Containers could be described in a similar way:

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
Graphical-First Design

MosaicOS is intended to be graphical from the beginning.

The graphical stack is not considered an optional layer added after the system boots. Instead, the normal boot target should include a minimal graphical environment composed of:

display server
compositor
window manager
input server
desktop shell
launcher
settings application
terminal
file manager

The first graphical prototype does not need hardware acceleration, advanced window effects, or full GPU support. A simple framebuffer-based environment running in QEMU would be enough for the first milestone.

Translators

Inspired by GNU Hurd, MosaicOS explores the idea of translators as user-space services that expose resources through the filesystem.

Examples:

/cloud/photos        -> cloud storage translator
/db/users            -> database translator
/net/http/example    -> network translator
/dev/sensors/temp    -> virtual device translator
/containers/devbox   -> container filesystem translator

A translator could allow applications and users to interact with complex resources using simple file operations.

Example configuration:

mounts:
  - target: /cloud
    type: translator
    translator: s3
    source: s3://my-bucket

  - target: /db/app
    type: translator
    translator: sqlite
    source: /data/app.db
Self-Healing Recovery Layer

A major goal of MosaicOS is to explore native disaster recovery as part of the operating system architecture.

The system should monitor critical services and attempt recovery when failures occur.

Example:

recovery:
  default_policy: restart_then_fallback

  snapshots:
    enabled: true
    backend: experimental
    before_updates: true

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

Possible recovery actions:

restart a failed service
switch to a fallback implementation
boot into a safe graphical mode
enter a recovery shell
rollback to a known-good configuration
collect crash logs and diagnostics
isolate a failing driver or service

The long-term idea is that a failure in one component should not necessarily cause a full system failure.

Container-Native Runtime

Applications should not run directly with broad access to the system. Instead, they should run inside isolated contexts.

Possible runtimes:

native MosaicOS runtime
Linux personality
BSD personality
WebAssembly runtime

Initial experiments may begin with native applications and WASM modules before attempting Linux ABI compatibility.

The long-term goal is to investigate whether Linux and BSD-like environments can be exposed as isolated personalities on top of a microkernel-based system.

Research Questions

This project is guided by several research questions:

Can a graphical desktop operating system be designed around isolation from the beginning?
Can system services be managed similarly to containers while remaining usable as an operating system?
Can a declarative configuration model simplify system composition and recovery?
Can Hurd-like translators be modernized for cloud storage, databases, APIs, and virtual devices?
Can automatic recovery be treated as a core OS feature instead of an external tool?
Can a microkernel-based graphical system provide a practical developer experience?
What are the trade-offs between performance, isolation, complexity, and compatibility?
Non-Goals

At the current stage, MosaicOS Lab does not aim to:

replace Linux or FreeBSD
run on real hardware immediately
provide full POSIX compatibility
run existing desktop applications
support modern GPUs
implement ZFS or Btrfs from scratch
provide production-grade security
become a general-purpose OS in the short term

This is a research and learning project first.

Possible Long-Term Directions

Future research may include:

Linux ABI compatibility
BSD personality layer
Wayland compatibility
ZFS or Btrfs integration as user-space storage services
FreeBSD-inspired network stack
user-space driver framework
graphical permission model
application sandboxing
package/runtime manager
safe update and rollback system
formal service contracts
paper publication
Project Status

This project is currently in the concept and research phase.

The first practical goal is to build a minimal L4Re-based laboratory environment running in QEMU and document the path toward a graphical, service-oriented, self-healing operating system prototype.

Why This Exists

MosaicOS Lab exists because operating systems are not only tools; they are ideas about how computers should be organized.

This project is a personal research journey into microkernels, service isolation, graphical systems, recovery mechanisms, runtime design, and the future of desktop operating systems.

The objective is not to build everything at once.

The objective is to learn deeply, prototype carefully, document honestly, and explore what a modern operating system could become.
