# MosaicOS Translators

Inspired by the GNU Hurd, MosaicOS utilizes **Translators** to bridge the gap between diverse resources and the standard filesystem interface.

## Definition
A translator is a user-space service that sits on top of a filesystem node (directory or file) and "translates" standard file operations (`read`, `write`, `ls`) into operations specific to the underlying resource.

## How They Work
When a process accesses a path managed by a translator:
1. The VFS (Virtual Filesystem) layer identifies the translator responsible for that path.
2. The VFS forwards the request to the translator via IPC.
3. The translator performs the necessary actions (e.g., an S3 GET request) and returns the result as a file buffer.

## Lifecycle
1. **Mount:** A translator is started and associated with a target directory in the system manifest.
2. **Expose:** The translator populates the target directory with virtual files/subdirectories.
3. **Unmount:** The translator is shut down, and the target directory returns to its previous state.

## Examples

### S3 Translator
Exposes an Amazon S3 bucket as a local folder.
- `ls /cloud/photos` -> Triggers `ListObjects` API call.
- `cp image.jpg /cloud/photos/` -> Triggers `PutObject` API call.

### SQLite Translator
Exposes database tables as virtual directories and rows as virtual files.
- `cat /db/users/1.json` -> Executes `SELECT * FROM users WHERE id=1`.

## Manifest Configuration

Translators are declared in the `mounts` section of the system manifest.

```yaml
mounts:
  - target: /cloud
    type: translator
    translator: s3
    source: s3://my-bucket
    capabilities:
      - net.client

  - target: /db/app
    type: translator
    translator: sqlite
    source: /data/app.db
```

## Benefits
- **Unified Interface:** Everything is a file. Applications don't need specialized SDKs for every cloud provider or database.
- **Isolation:** Each translator is a separate process. A crash in the S3 translator only affects `/cloud`.
- **Flexibility:** New types of storage or APIs can be added without kernel changes.
