# MosaicOS GUI Protocol

The authoritative Milestone 5 protocol specification lives in
`docs/graphics-stack.md`.

This directory tracks protocol history and implementation notes for the graphics
stack. Version `0.1` defines:

- ARGB8888 surfaces
- client-side rendering
- server-side compositing
- server-side decorations
- `DisplayRequest`
- `CompositorRequest`
- `InputEvent`
- `CompositorEvent`
- compositor crash recovery through `safe-gui`

The current lab implementation exercises these concepts through serial-visible
L4Re services while real framebuffer mapping is still pending.
