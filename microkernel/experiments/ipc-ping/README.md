# IPC Ping Experiment

This experiment validates raw L4Re IPC before MosaicOS introduces any service
manager abstractions.

## Components

- `ping-receiver`: receives one message through the `ping_server` IPC gate and
  prints it to the serial log.
- `ping-sender`: looks up the same `ping_server` capability, sends
  `MosaicOS Lab: IPC ping`, waits for a reply, and exits.

The capability wiring is declared in `tools/lab/conf/mosaicos-ipc-ping.cfg`.

## Build

From the repository root:

```bash
./tools/lab/build-kernel.sh
./tools/lab/build-l4re.sh
./tools/lab/build-hello.sh
```

## Run

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
