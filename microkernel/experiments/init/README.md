# mosaic-init Experiment

This experiment implements the first `mosaic-init` service manager. It reads a
small YAML-style manifest from L4Re ROM, validates service definitions, resolves
`requires` dependencies, starts eligible services by dependency round, and prints
an observable status table.

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
