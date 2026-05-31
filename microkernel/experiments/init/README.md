# mosaic-init Experiment

This experiment starts Milestone 2 by introducing the first `mosaic-init`
binary. It reads a single-service manifest from L4Re ROM, validates the required
fields, and uses its Ned command capability to start the declared service binary.

The initial manifest is a deliberately small YAML subset:

```yaml
services:
  hello:
    binary: rom/mosaic-hello
    restart: never
```

Run it through the lab entry:

```bash
./tools/lab/build-hello.sh
./tools/lab/run-qemu.sh mosaicos-init
```

Expected serial output includes:

```text
mosaic-init: starting
mosaic-init: service 'hello'
mosaic-init: starting service 'hello'
mosaic-init: service 'hello' started
MosaicOS Lab: hello from L4Re
```
