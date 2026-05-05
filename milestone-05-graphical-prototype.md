# Milestone 5 — Graphical Prototype

## Context

MosaicOS is graphical by default. The graphical stack is not an optional layer — it is a core boot target. This milestone produces the first visible graphical output: a framebuffer display service, a minimal compositor, a basic window, and keyboard/mouse input — all running in QEMU and integrated with the recovery system from Milestone 4.

No GPU acceleration. No hardware compositing. Framebuffer only.

## Objective

Implement the minimal graphical stack: display-server (framebuffer), compositor, gui-protocol (IPC), input-server, and one graphical application that draws a window and receives input. Integrate crash recovery: if the compositor crashes, the system falls back to safe-gui mode.

## Prerequisites

- Milestone 4 complete: recovery system works, guardian handles fallback services
- Recovery shell works over serial
- docs/graphics-stack.md defines component responsibilities and gui-protocol

## Deliverables

### 1. display-server — framebuffer owner

Location: `graphics/display-server/`

Language: Rust or C

Responsibilities:
- Obtain the QEMU framebuffer via L4Re's framebuffer capability (`l4re::Fb`)
- Expose a shared memory buffer to the compositor
- Handle mode information: width, height, bits per pixel
- Accept `Flip` commands from the compositor (double buffering)

IPC interface (define in `sdk/ipc-api/display.rs`):
```rust
enum DisplayRequest {
    GetInfo,                    // → DisplayInfo { width, height, bpp, pitch }
    MapBuffer { slot: u8 },     // → shared memory cap for back buffer
    Flip { slot: u8 },          // compositor requests buffer swap
    Shutdown,
}
```

For QEMU, the framebuffer is typically obtained via the `vesa` or `fb` L4Re driver. Check the L4Re package `drivers/fb` or use QEMU's `-device VGA` with the L4Re framebuffer server.

### 2. compositor — window composition

Location: `graphics/compositor/`

Language: Rust

Responsibilities:
- Receive window buffers from client applications via shared memory
- Maintain a window list with z-order
- Composite all window buffers onto the back buffer
- Request a `Flip` from the display-server after each frame
- Route input events from the input-server to the focused window
- Enforce basic window management: one focused window at a time

IPC interface (define in `sdk/ipc-api/compositor.rs`):
```rust
enum CompositorRequest {
    CreateWindow { width: u32, height: u32, title: [u8; 64] }, // → WindowId, shared mem cap
    DestroyWindow { window_id: u32 },
    CommitFrame { window_id: u32 },     // app signals it has drawn a new frame
    MoveWindow { window_id: u32, x: i32, y: i32 },
    ResizeWindow { window_id: u32, width: u32, height: u32 },
    SetFocus { window_id: u32 },
    GetWindowList,                      // → Vec<WindowInfo>
}
```

The compositor composites at a fixed rate (aim for 30 FPS in QEMU; exact rate is not critical).

### 3. gui-protocol — the IPC contract

Location: `sdk/gui-sdk/`

The gui-protocol is the complete IPC contract between applications and the compositor. It must be documented and versioned.

Define in `sdk/gui-sdk/protocol.rs`:
- All request and response types for compositor and input-server
- Window buffer layout (pixel format: ARGB8888 or RGB888)
- Input event types

This is the interface all graphical applications will use. Keep it minimal but extensible.

### 4. input-server — input event router

Location: `graphics/input-server/`

Language: Rust

Responsibilities:
- Read raw keyboard and mouse events from QEMU's input device via L4Re
- Translate to MosaicOS input event format
- Route events to the compositor (which forwards to the focused window)

Input event format (define in `sdk/gui-sdk/input.rs`):
```rust
enum InputEvent {
    KeyDown { keycode: u32, scancode: u32, modifiers: u8 },
    KeyUp   { keycode: u32, scancode: u32, modifiers: u8 },
    MouseMove { dx: i32, dy: i32 },
    MouseButton { button: u8, pressed: bool },
    MouseScroll { delta: i32 },
}
```

For QEMU keyboard input: use L4Re's `input` server or map the QEMU PS/2 keyboard directly via the L4Re I/O server.

### 5. First graphical application — mosaic-hello

Location: `shell/apps/mosaic-hello/`

Language: Rust

A minimal graphical application that:
- Connects to the compositor via IPC
- Creates a 400×300 window titled "MosaicOS Hello"
- Draws a solid background color (e.g., dark purple `#1a1a2e`)
- Draws a centered text label "MosaicOS Lab" in white pixels (bitmap font)
- Responds to keyboard input: press `Q` to exit
- Logs all received input events via logd

This application is the first visual proof that the graphical stack works end-to-end.

Pixel drawing must be done manually into the shared memory buffer. No graphics library. Simple rectangle fill and bitmap font rendering are sufficient.

Provide a minimal bitmap font (8×8 or 8×16 pixels per character) as a const array in `sdk/gui-sdk/font.rs`.

### 6. Safe GUI mode

Location: `recovery/safe-gui/`

Language: Rust

The safe-gui is a fallback graphical environment activated when the compositor crashes beyond recovery. It must:
- Use the display-server directly (bypassing the crashed compositor)
- Draw a simple screen: dark background, centered warning message
- Show: "MosaicOS — Safe Mode. Compositor unavailable."
- Show current service states from the registry
- Provide a button (or keyboard shortcut) to launch the recovery shell
- Work without depending on any service that may have crashed

The safe-gui must be registered as the `fallback` for the compositor in the manifest.

### 7. Integration with recovery system

Update `boot/manifests/development.yaml` to include the graphical services:

```yaml
services:
  display:
    binary: /system/graphics/displayd
    restart: on-failure
    capabilities:
      - fb.access
    recovery:
      max_restarts: 2
      fallback: framebuffer-display

  input:
    binary: /system/graphics/inputd
    restart: on-failure
    requires:
      - logger
    capabilities:
      - input.keyboard
      - input.mouse

  compositor:
    binary: /system/graphics/compositord
    restart: on-failure
    requires:
      - display
      - input
    recovery:
      max_restarts: 3
      fallback: safe-gui
```

Test: kill the compositor process. The guardian must detect the failure, attempt restarts, and eventually start safe-gui after `max_restarts` is exceeded.

## File layout

```
graphics/
├── display-server/
│   └── src/main.rs
├── compositor/
│   └── src/main.rs
├── input-server/
│   └── src/main.rs
└── gui-protocol/
    └── README.md              (protocol documentation)

sdk/
└── gui-sdk/
    ├── src/
    │   ├── lib.rs
    │   ├── protocol.rs
    │   ├── input.rs
    │   └── font.rs            (bitmap font)
    └── Cargo.toml

shell/
└── apps/
    └── mosaic-hello/
        └── src/main.rs

recovery/
└── safe-gui/
    └── src/main.rs

sdk/
└── ipc-api/
    ├── display.rs             (new)
    └── compositor.rs          (new)
```

## QEMU configuration for graphics

Update `tools/lab/run-qemu.sh` to enable graphical output:

```bash
qemu-system-x86_64 \
  -m 512M \
  -device VGA,vgamem_mb=16 \
  -device ps2-kbd \
  -device ps2-mouse \
  -serial stdio \
  ...
```

Or use `-display sdl` / `-display gtk` for a windowed QEMU session during development.

## Architectural constraints

- The compositor owns the screen. No application writes to the framebuffer directly.
- The display-server owns the framebuffer. The compositor accesses it only via the defined IPC interface.
- Input events flow: hardware → input-server → compositor → focused window. No application registers directly with the input-server.
- The safe-gui must not depend on the compositor. It writes directly to the display-server.
- The gui-sdk is the only way applications interact with the graphical stack. No application should use display or input IPC directly.
- Crash recovery integration is mandatory. This milestone is not complete without the compositor crash → safe-gui path working.

## Completion criteria

This milestone is complete when:

- [ ] QEMU boots with a visible graphical display (not just serial)
- [ ] `mosaic-hello` window appears on screen with background color and text label
- [ ] Pressing `Q` in the focused window exits the application cleanly
- [ ] Mouse movement is visible (cursor or window title changes)
- [ ] Killing the compositor triggers recovery: restarts up to 3 times, then starts safe-gui
- [ ] Safe-gui displays "Safe Mode" message on screen after compositor exhausts restarts
- [ ] `mosaicctl status` still works (serial) while graphical stack is running
- [ ] All graphical services are listed in `mosaicctl status` with correct states

## Notes for the AI

- Start with the simplest possible rendering: fill the entire framebuffer with a solid color. Once that works, add the window abstraction.
- The bitmap font can be an 8×8 or 8×16 pixel font encoded as a byte array. There are many public domain 8×8 fonts available (e.g., IBM CP437). The font needs to render ASCII printable characters only.
- Double-buffering (drawing to a back buffer then flipping) prevents tearing. Implement it from the start — it is not significantly more complex than single-buffer.
- The compositor's composition loop: clear back buffer → draw each window in z-order → flip. At 30 FPS this is one composition every ~33ms.
- For the first iteration, the compositor does not need to handle window overlapping correctly (transparency, clipping). Just draw windows in z-order with no alpha blending.
- L4Re shared memory: use `l4re::Mem_alloc` to allocate a shared data space, then pass the capability to the application. The application maps it and draws into it directly.
- The safe-gui should be as simple as possible. A few rectangles and some text on a dark background is sufficient.
