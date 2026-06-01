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
