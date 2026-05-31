# MosaicOS Lab - L4Re Setup Guide

This guide establishes the Milestone 1 laboratory and the first Milestone 2
experiment: build Fiasco.OC and L4Re, boot L4Re in QEMU, run a hello task,
validate basic IPC between two tasks, and run the minimal `mosaic-init` flow.

## Host Requirements

- Linux x86-64 host, tested targets are Ubuntu 24.04 and Arch/CachyOS.
- `git`, `make`, GCC/G++ 11 to 15 or Clang/LLD, `flex`, `bison`, `gawk`,
  `pkg-config`/`pkgconf`, `python3`.
- `qemu-system-x86_64`.
- GRUB image tooling: `grub-pc-bin`, `mtools`, `xorriso`.

Install dependencies on Ubuntu/Debian or Arch/CachyOS with:

```bash
./tools/lab/setup.sh
```

Or use the container image:

```bash
docker build -f tools/lab/Dockerfile -t klugeos-l4re-lab .
docker run --rm -it -v "$PWD:/workspace/klugeos" klugeos-l4re-lab
```

## Workspace Layout

The scripts are idempotent. By default they use:

- `.lab/ham` and `.lab/.ham` for the L4Re Ham project manager.
- `.lab/l4` for the complete L4Re source tree.
- `.lab/fiasco` for the Fiasco.OC source tree.
- `.lab/build/fiasco` for the kernel build.
- `.lab/build/l4re` for the L4Re userland build.

Override locations with environment variables:

```bash
LAB_WORKDIR=/tmp/klugeos-lab ./tools/lab/build-kernel.sh
L4RE_DIR=/opt/l4re-core BUILD_DIR=/opt/l4re-build ./tools/lab/build-l4re.sh
```

## Build

From the repository root:

```bash
./tools/lab/build-kernel.sh
./tools/lab/build-l4re.sh
./tools/lab/build-hello.sh
```

`build-kernel.sh` installs the L4Re source tree with Ham when missing, then
builds the Fiasco.OC kernel. `build-l4re.sh` configures and builds L4Re
userland. `build-hello.sh` builds the local experiments using the L4Re BID make
system.

## Run Hello

```bash
./tools/lab/run-qemu.sh mosaicos-hello
```

Expected serial output includes:

```text
MosaicOS Lab: hello from L4Re
```

The runner delegates to L4Re's `make qemu` target with these required defaults:

- `-monitor none`
- `-serial stdio`
- `-m 512`
- `-M q35`
- `-nographic`
- `-no-reboot`

## Run IPC Ping

```bash
./tools/lab/run-qemu.sh mosaicos-ipc-ping
```

Expected serial output includes:

```text
Receiver: Waiting for message...
Sender: Sending message to receiver...
Receiver: Received IPC from label ...
MosaicOS Lab: IPC ping
Sender: Message sent and reply received.
```

## Run Minimal Init

```bash
./tools/lab/run-qemu.sh mosaicos-init
```

Expected serial output includes:

```text
mosaic-init: starting
mosaic-init: loading manifest rom/mosaic-init.manifest
mosaic-init: service 'hello'
mosaic-init: binary 'rom/mosaic-hello'
mosaic-init: starting service 'hello'
mosaic-init: service 'hello' started
MosaicOS Lab: hello from L4Re
```

The manifest is `tools/lab/conf/mosaic-init.manifest` and uses the same
YAML-style `services` shape described by the service-model documentation, limited
to one service for this milestone. `mosaic-init` reads the manifest and uses its
Ned command capability to request startup of the declared binary.

## Boot Configuration

The lab boot entries live in `tools/lab/conf/mosaicos-lab.list`.

- `mosaicos-hello` boots `moe`, starts `ned`, then runs `mosaic-hello`.
- `mosaicos-init` boots `moe`, starts `ned`, then runs `mosaic-init` with a
  single-service manifest.
- `mosaicos-ipc-ping` boots `moe`, starts `ned`, creates a `ping_server`
  IPC gate, gives server rights to `ping-receiver`, and gives client rights to
  `ping-sender`.

The Lua configs are:

- `tools/lab/conf/mosaicos-hello.cfg`
- `tools/lab/conf/mosaicos-init.cfg`
- `tools/lab/conf/mosaicos-ipc-ping.cfg`

To add a new task:

1. Add a new experiment directory under `microkernel/experiments/`.
2. Add a BID Makefile with `TARGET` and source files.
3. Build it from `tools/lab/build-hello.sh` or a new script.
4. Add the binary and optional Lua config to `tools/lab/conf/mosaicos-lab.list`.
5. Run with `./tools/lab/run-qemu.sh <entry-name>`.

## Common Errors

- **`qemu-system-x86_64: command not found`:** install `qemu-system-x86`.
- **GRUB/image generation errors:** install `grub-pc-bin`, `mtools`, and
  `xorriso`.
- **`../../mk/project.mk: No such file or directory`:** the checkout is only
  `l4re-core`, not the full L4Re tree. Use the provided scripts so Ham
  synchronizes `.lab/l4` and `.lab/fiasco`.
- **Missing generated L4Re files:** run `./tools/lab/build-l4re.sh` before
  building experiments.
- **`gcc-16 is not supported`:** install `gcc-14 g++-14`, Arch/CachyOS
  `gcc14`, or use Clang/LLD 21 or older. `env.sh` selects GCC 11-15 when
  present and otherwise falls back to a supported Clang/LLD.
- **Module not found at boot:** confirm the binary name appears in
  `tools/lab/conf/mosaicos-lab.list` and that `MODULE_SEARCH_PATH` includes the
  experiment directory. `run-qemu.sh` sets this automatically.
- **No serial output:** keep the default `QEMU_OPTIONS`; replacing them must
  preserve `-monitor none`, `-serial stdio`, `-M q35`, and a headless display
  option for CI.
