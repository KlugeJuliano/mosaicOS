# MosaicOS Graphics Stack

MosaicOS is designed as a graphical-first operating system. The graphical environment is not an optional layer but a core boot target.

## Components

### 1. Display Server (`displayd`)
- **Responsibility:** Direct interaction with hardware framebuffers or GPU drivers. It owns the physical screen output.
- **IPC Role:** Provides a pixel-pushing interface to the compositor.

### 2. Compositor (`compositord`)
- **Responsibility:** Managing window buffers, performing alpha blending, and coordinating window visibility.
- **IPC Role:** Receives frame buffers from applications and sends the final scene to the display server.

### 3. Window Manager (`wmd`)
- **Responsibility:** Window placement policies, focus management, decorations (borders, title bars), and workspace organization.
- **Isolation:** Separated from the compositor to allow different desktop metaphors without changing the core rendering engine.

### 4. Input Server (`inputd`)
- **Responsibility:** Receiving raw input events from the kernel (mouse, keyboard, touch) and routing them to the focused window or the compositor.

## GUI Protocol
Applications interact with the graphics stack via a dedicated IPC protocol.
- **Buffer Sharing:** Apps share memory regions containing window frames (protected by capabilities).
- **Event Stream:** Apps receive input events via an asynchronous message queue.

## Recovery Path
The graphics stack is a common point of failure. MosaicOS addresses this through specific recovery strategies:

1. **Compositor Crash:**
   - The `Guardian` detects the crash.
   - It attempts to restart the compositor.
   - If restart fails, the system falls back to `safe-gui` (a minimal, high-reliability interface).
2. **Display Driver Failure:**
   - Fall back to a generic framebuffer driver if available.
3. **App Hang:**
   - The compositor can forcibly close windows of unresponsive applications.
