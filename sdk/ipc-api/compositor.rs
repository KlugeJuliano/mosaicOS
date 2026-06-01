pub enum SurfaceRole {
    Toplevel,
    Popup,
    Background,
}

pub enum CompositorRequest {
    CreateWindow {
        content_width: u32,
        content_height: u32,
        title: [u8; 64],
        role: SurfaceRole,
    },
    DestroyWindow { window_id: u32 },
    CommitFrame { window_id: u32 },
    MoveWindow { window_id: u32, x: i32, y: i32 },
    ResizeWindow { window_id: u32, width: u32, height: u32 },
    GetWindowList,
    Shutdown,
}

pub struct WindowId(pub u32);

pub struct WindowInfo {
    pub window_id: u32,
    pub title: [u8; 64],
    pub x: i32,
    pub y: i32,
    pub content_width: u32,
    pub content_height: u32,
    pub focused: bool,
}
