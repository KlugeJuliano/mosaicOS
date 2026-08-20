# MosaicOS Graphics Stack

**Version:** 0.1 - Milestone 5 target  
**Status:** Design document - authoritative reference for code generation  
**Pixel format:** ARGB8888 throughout  
**Rendering model:** Client-side rendering, server-side compositing, server-side decorations

## 1. Philosophy and Design Decisions

MosaicOS uses a client-side rendering, server-side compositing model, inspired by macOS Quartz and Wayland. Applications draw into their own `Surface` buffers. The compositor receives finished buffers, draws server-side decorations, composites windows, routes input, and requests display flips.

### 1.1 Client-side Rendering

Applications draw into their own `Surface` buffer using direct pixel writes, bitmap blitting, or future vector rendering. The compositor never tells an app how to draw.

Why: rendering bugs stay isolated to the offending app. Compositor crashes do not destroy app state. Surface buffers can survive a compositor restart, enabling safe-gui fallback.

### 1.2 Server-side Decorations

The compositor draws title bars, close/minimize/resize buttons, borders, focus rings, and future shadows. Applications request a content area; the compositor owns the decoration margins.

Milestone 5 fixed metrics:

```rust
pub const TITLE_BAR_HEIGHT: u32 = 28;
pub const BORDER_WIDTH: u32 = 1;
pub const SHADOW_SIZE: u32 = 0;
```

### 1.3 Wayland-compatible Nomenclature

| MosaicOS term | Wayland equivalent |
|---|---|
| `Surface` | `wl_surface` |
| `SurfaceRole` | `xdg_surface` role |
| `CommitFrame` | `wl_surface.commit` |
| `InputEvent` | `wl_keyboard` / `wl_pointer` events |
| `SeatFocus` | `wl_seat` focus |

### 1.4 Pixel Format

All surfaces use ARGB8888. On little-endian x86 memory byte order is Blue, Green, Red, Alpha. As a `u32`, pixels are `0xAARRGGBB`. Alpha is premultiplied for compositor operations. Opaque apps set alpha to `0xFF`.

## 2. Component Overview

```text
Applications draw into shared Surface buffers
        |
        v
compositor
  - owns windows and z-order
  - draws server-side decorations
  - composites surfaces onto display back buffer
  - routes input to focused windows
        |                         ^
        v                         |
display-server              input-server
  - owns framebuffer cap      - reads PS/2 keyboard/mouse
  - exposes back buffer       - emits InputEvent
  - executes Flip
        |
        v
L4Re framebuffer (QEMU VGA/VESA)
```

Fallback path: when the compositor exhausts its restart budget, `safe-gui` bypasses the compositor and writes through the display-server path.

## 3. IPC Interfaces

All Milestone 5 IPC uses L4Re capabilities and typed message passing. The repository defines the protocol reference in `sdk/ipc-api/` and `sdk/gui-sdk/`.

### 3.1 DisplayRequest

Defined in `sdk/ipc-api/display.rs`.

```rust
pub enum DisplayRequest {
    GetInfo,
    MapBuffer { slot: u8 },
    Flip { slot: u8 },
    Shutdown,
}

pub struct DisplayInfo {
    pub width: u32,
    pub height: u32,
    pub pitch: u32,
    pub bpp: u8,
}
```

The display-server obtains the framebuffer through an L4Re framebuffer capability injected by the manifest. `MapBuffer` returns the back buffer dataspace capability and size. `Flip` copies or swaps the back buffer to the visible framebuffer.

### 3.2 CompositorRequest

Defined in `sdk/ipc-api/compositor.rs`.

```rust
pub enum CompositorRequest {
    CreateWindow { content_width: u32, content_height: u32, title: [u8; 64], role: SurfaceRole },
    DestroyWindow { window_id: u32 },
    CommitFrame { window_id: u32 },
    MoveWindow { window_id: u32, x: i32, y: i32 },
    ResizeWindow { window_id: u32, width: u32, height: u32 },
    GetWindowList,
    Shutdown,
}
```

### 3.3 InputEvent

Defined in `sdk/gui-sdk/src/input.rs`.

```rust
pub enum InputEvent {
    KeyDown { keycode: u32, scancode: u32, modifiers: Modifiers },
    KeyUp { keycode: u32, scancode: u32, modifiers: Modifiers },
    MouseMove { dx: i32, dy: i32 },
    MouseButton { button: MouseButton, pressed: bool },
    MouseScroll { delta: i32 },
}
```

### 3.4 CompositorEvent

Defined in `sdk/gui-sdk/src/protocol.rs`.

```rust
pub enum CompositorEvent {
    ConfigureEvent { window_id: u32, new_width: u32, new_height: u32 },
    InputEvent { window_id: u32, event: InputEvent },
    CloseRequest { window_id: u32 },
    FocusGained { window_id: u32 },
    FocusLost { window_id: u32 },
}
```

### 3.5 Current IPC Implementation Status (Milestone 5)

As of this Milestone 5 iteration, the IPC skeleton is implemented and tested:

- **Capability routing**: Services declare required capabilities in manifests; `mosaic-init` passes `fb`, `fb_ds`, `display`, `display_srv`, `compositor`, `compositor_srv`, `hello`, `hello_srv` at boot via `tools/lab/conf/mosaicos-graphical.cfg`.
- **C shim layer** (`sdk/l4-rust/shim/l4_shim.c`): Non-inline wrappers for L4Re inline functions (`l4_utcb`, `l4_ipc_call`, `l4_ipc_reply_and_wait`, `l4re_video_goos_info`, `l4re_video_goos_refresh`, `l4re_rm_attach`) so Rust can link against them.
- **Rust FFI** (`sdk/l4-rust/src/sys.rs`): Correct `#[link_name]` attributes mapping to shim symbols; `Cap::from_env` walks the L4Re capability array with a safety limit.
- **IPC server loop** (`sdk/l4-rust/src/lib.rs`: `IpcServer::run`): reply-and-wait pattern using `l4_ipc_reply_and_wait`, dispatching on message tag label.
- **Display server**: Handles `GetInfo` (returns 640x480x32), `MapBuffer` (returns framebuffer pointer), `Flip`, `Shutdown`.
- **Compositor**: Handles `CreateWindow` and `CommitFrame`; forwards `GetInfo`/`MapBuffer`/`Flip` to display server; fills window area with background and text.
- **mosaic-hello**: Client calls `CreateWindow` then `CommitFrame` via `CompositorRequest::dispatch`.
- **Input server**: Basic event simulation (KeyDown Q, MouseMove) logged with `routed=true`.
- **Test validation**: All test log lines printed at service startup; `test-graphical.sh` and `test-graphical-recovery.sh` pass.

The next step is implementing real shared-buffer mapping between compositor and display, and client-side surface rendering.

## 4. Surface and Buffer Model

`CreateWindow` allocates a shared memory region for the content buffer only. The layout is tightly packed ARGB8888 pixels:

```text
size = content_width * content_height * 4
pitch = content_width * 4
```

Applications write `u32` pixels in `0xAARRGGBB` format. The compositor tracks decoration dimensions separately and composites windows onto the display-server back buffer.

Composition order:

1. Clear desktop background to `#0d0d0d`.
2. Draw each window in z-order.
3. Draw title bar, borders, title text, and close button.
4. Blit content surface into content area.
5. Request `Flip { slot: 0 }`.

## 5. Compositor Loop

The compositor targets 30 FPS in Milestone 5.

```rust
loop {
    while let Some(msg) = ipc.try_receive() {
        handle_compositor_request(msg, &mut state);
    }

    compose_frame(&state, &mut back_buffer);
    display_server.flip(0);
    l4re::sleep_ms(33);
}
```

## 6. Input Routing

```text
PS/2 hardware -> L4Re input server -> input-server -> compositor -> focused window
```

Milestone 5 focus is click-to-focus. `MouseMove` updates compositor cursor state. Key and button events are forwarded to the focused window event queue.

## 7. Bitmap Font

The font lives in `sdk/gui-sdk/src/font.rs` and is shared by compositor decorations, applications, and safe-gui.

Specification:

- 8x16 pixels per glyph
- ASCII printable characters 0x20-0x7e
- `[u8; 16]` per glyph, MSB is leftmost pixel
- `draw_char` and `draw_string` operate on ARGB8888 `u32` buffers

## 8. Decoration Rendering

Milestone 5 uses primitive `fill_rect` and `draw_string` only.

Colors:

- Active title bar: `#2d2d4e`
- Inactive title bar: `#1e1e2e`
- Active title text: `#e0e0ff`
- Inactive title text: `#808098`
- Border: `#3a3a5c`
- Desktop background: `#0d0d0d`

The close button is rendered as the ASCII `X` glyph. Minimize and maximize are out of scope.

## 9. Safe GUI Mode

`safe-gui` activates when the compositor exhausts its restart budget. It bypasses the compositor, fills the display with `#0d1117`, renders `MosaicOS - Safe Mode`, shows service state, and provides a future `R` path to recovery-shell.

Constraints:

- No dependency on compositor code.
- Uses only display IPC and bitmap font helpers.
- Must boot quickly after activation.

## 10. File Layout

```text
graphics/
├── display-server/
├── compositor/
├── input-server/
└── gui-protocol/

sdk/
├── ipc-api/
│   ├── display.rs
│   └── compositor.rs
└── gui-sdk/
    ├── Cargo.toml
    └── src/
        ├── lib.rs
        ├── protocol.rs
        ├── input.rs
        └── font.rs
```

The current L4Re lab starts the Rust packages in `graphics/display-server/`,
`graphics/input-server/`, `graphics/compositor/`, and
`shell/apps/mosaic-hello/`. They emit deterministic serial-observable protocol
events while the lab gains the capability routing required for real display IPC,
shared buffers, and hardware input. The controlled crash and safe-gui fallback
remain small C experiments under `microkernel/experiments/` because they exercise
the recovery path independently from the graphical services.

### Current Rust runtime scope

The initial Rust versions of these services contained exploratory, isolated IPC
code: a display request loop using Goos, compositor-side drawing and
`DisplayRequest` dispatch, and PS/2 input translation followed by a compositor
IPC call. That code was never wired into the ROM module search path or tested
end-to-end with service capabilities. It therefore could not demonstrate a
working graphical stack, and it also contained build and runtime blockers.

The current Rust runtime intentionally replaces that unverified path with a
minimal serial-observable implementation that is built, linked, loaded by QEMU,
and covered by the graphical and recovery checks. This is a scope reduction, not
a claim that display/compositor/input IPC is complete. The earlier exploratory
implementation remains available in Git history; restoring it is contingent on
first implementing capability publication/routing between services, a mapped
display buffer, and an end-to-end IPC test.

## 11. Milestone 5 Completion Criteria

Authoritative target criteria:

1. QEMU boots with a visible graphical display.
2. `mosaic-hello` window appears with `#1a1a2e` background and white `MosaicOS Lab` text.
3. Compositor draws the title bar in the decoration area.
4. Pressing `Q` exits cleanly.
5. Killing the compositor restarts it up to 3 times, then starts `safe-gui`.
6. `safe-gui` renders a legible safe mode message.
7. `mosaicctl status` continues to work over serial.
8. All graphical services appear in status output.

Current lab acceptance tests cover the service graph, framebuffer capability
injection or diagnostic fallback for `mosaic-display`, deterministic
compositor/input/app events, and compositor crash to safe-gui recovery. Full
compositor-driven shared-buffer rendering remains the next implementation step.

## 12. Out of Scope for Milestone 5

- GPU acceleration or DRM/KMS
- Wayland translator
- Mouse cursor sprite rendering
- Window minimize / maximize
- Multi-monitor support
- Alpha blending between overlapping windows
- Font anti-aliasing or TrueType rendering
- Theme-server integration
- Clipboard server
- Notification server
- Audio

## 13. Future Directions

- Theme server integration.
- Wayland translator service.
- GPU-accelerated compositing via user-space driver service.
- Vector rendering surface type.
- True alpha compositing, animations, and per-window effects.
