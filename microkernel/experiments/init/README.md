# mosaic-init Experiment

This experiment implements the first `mosaic-init` service manager. It reads a
small YAML-style manifest from L4Re ROM, validates service definitions, resolves
`requires` dependencies, starts eligible services by dependency round, and prints
an observable status table.

Manifest validation rejects unknown dependencies, self-dependencies, duplicate
dependencies, dependency cycles, missing binaries, unsafe service names/paths,
and unsupported restart policies.

For recovery experiments, services with `restart: on-failure` or non-zero
`recovery.max_restarts` are launched with exit monitoring. `mosaic-init` waits on
the Ned task handle, restarts failed services up to `max_restarts`, and marks the
service `failed` when the retry budget is exhausted.

The manifest subset supports multiple services, `binary`, `restart`, and
`requires`:

```yaml
services:
  logger:
    binary: rom/mosaic-log
    restart: never

  hello:
    binary: rom/mosaic-hello
    restart: never
    requires:
      - logger
```

Run it through the lab entry:

```bash
./tools/lab/build-hello.sh
./tools/lab/run-qemu.sh mosaicos-init
```

Or run the automated lab check:

```bash
./tools/lab/test-service-manager.sh
```

Run the recovery prototype:

```bash
./tools/lab/test-recovery.sh
```

Expected serial output includes:

```text
mosaic-init: starting
mosaic-init: loaded 3 service(s)
mosaic-init: startup round 1
mosaic-init: starting service 'logger'
mosaic-init: startup round 2
mosaic-init: starting service 'hello'
mosaic-init: starting service 'status'
mosaic-init: status logger       running
mosaic-init: status hello        running
mosaic-init: status status       running
MosaicOS Lab: log service online
MosaicOS Lab: hello from L4Re
MosaicOS Lab: status service online
```
