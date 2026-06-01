use crate::input::InputEvent;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum SurfaceRole {
    Toplevel,
    Popup,
    Background,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum CompositorEvent {
    ConfigureEvent { window_id: u32, new_width: u32, new_height: u32 },
    InputEvent { window_id: u32, event: InputEvent },
    CloseRequest { window_id: u32 },
    FocusGained { window_id: u32 },
    FocusLost { window_id: u32 },
}
