use core::ffi::c_char;

pub type l4_cap_idx_t = usize;
pub type l4_umword_t = usize;
pub type l4_mword_t = isize;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct l4_msgtag_t {
    pub raw: l4_mword_t,
}

#[repr(C)]
pub struct l4_utcb_t {
    // This is architecture dependent, but we only care about MRs for now
    pub mr: [l4_umword_t; 64],
}

pub const L4_CAP_SHIFT: usize = 12;
pub const L4_INVALID_CAP: l4_cap_idx_t = !0;

extern "C" {
    pub fn l4_utcb() -> *mut l4_utcb_t;
    pub fn l4re_env_get_cap(name: *const c_char) -> l4_cap_idx_t;
    pub fn l4_ipc_call(
        dest: l4_cap_idx_t,
        utcb: *mut l4_utcb_t,
        tag: l4_msgtag_t,
        timeout: usize,
    ) -> l4_msgtag_t;

    pub fn l4_ipc_reply_and_wait(
        utcb: *mut l4_utcb_t,
        tag: l4_msgtag_t,
        src: *mut l4_cap_idx_t,
        timeout: usize,
    ) -> l4_msgtag_t;

    pub fn l4re_ds_map(
        ds: l4_cap_idx_t,
        offset: l4_umword_t,
        flags: l4_umword_t,
        local_addr: *mut l4_umword_t,
        min_addr: l4_umword_t,
        max_addr: l4_umword_t,
    ) -> i32;

    pub fn l4re_rm_attach(
        addr: *mut *mut core::ffi::c_void,
        size: l4_umword_t,
        flags: l4_umword_t,
        ds: l4_cap_idx_t,
        offset: l4_umword_t,
        align: u8,
    ) -> i32;

    pub fn l4re_video_goos_get_info(
        goos: l4_cap_idx_t,
        width: *mut u32,
        height: *mut u32,
        bpp: *mut u8,
        pitch: *mut u32,
    ) -> i32;

    pub fn l4re_video_goos_refresh(
        goos: l4_cap_idx_t,
        x: u32, y: u32, w: u32, h: u32
    ) -> i32;

    pub fn l4re_input_get_event(
        input: l4_cap_idx_t,
        event: *mut l4_input_event_t,
    ) -> i32;

    pub fn l4_sleep(ms: u32);
}

#[repr(C)]
pub struct l4_input_event_t {
    pub type_: u16,
    pub code: u16,
    pub value: i32,
    pub timestamp: u64,
}

impl l4_msgtag_t {
    pub fn new(label: l4_mword_t, words: u32, items: u32, flags: u32) -> Self {
        Self {
            raw: (label << 16) | ((words as l4_mword_t) & 0x3f) | (((items as l4_mword_t) & 0x3f) << 6) | (((flags as l4_mword_t) & 0xf) << 12),
        }
    }

    pub fn label(&self) -> l4_mword_t {
        self.raw >> 16
    }

    pub fn words(&self) -> u32 {
        (self.raw & 0x3f) as u32
    }
}
