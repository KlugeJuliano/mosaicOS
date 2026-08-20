#![no_std]

pub extern crate alloc;

pub mod sys;
pub mod bump_alloc;

use core::ffi::CStr;
use sys::*;

#[derive(Clone, Copy)]
pub struct Cap(pub l4_cap_idx_t);

impl Cap {
    pub fn from_env(name: &CStr) -> Option<Self> {
        let cap = unsafe { l4re_env_get_cap(name.as_ptr()) };
        if cap == L4_INVALID_CAP { None } else { Some(Cap(cap)) }
    }

    pub fn is_valid(&self) -> bool {
        self.0 != L4_INVALID_CAP
    }
}

pub fn utcb() -> &'static mut l4_utcb_t {
    unsafe { &mut *l4_utcb() }
}

pub fn ipc_call(dest: Cap, tag: l4_msgtag_t) -> l4_msgtag_t {
    unsafe { l4_ipc_call(dest.0, l4_utcb(), tag, 0) }
}

pub fn ipc_reply_and_wait(tag: l4_msgtag_t, src: &mut l4_cap_idx_t) -> l4_msgtag_t {
    unsafe { l4_ipc_reply_and_wait(l4_utcb(), tag, src as *mut _, 0) }
}

pub struct Dataspace(pub Cap);

impl Dataspace {
    pub fn map(&self, offset: usize, flags: usize) -> Result<*mut u8, i32> {
        let mut local_addr = 0;
        let res = unsafe {
            l4re_ds_map(self.0 .0, offset, flags, &mut local_addr, 0, !0)
        };
        if res < 0 {
            Err(res)
        } else {
            Ok(local_addr as *mut u8)
        }
    }

    pub fn attach(&self, size: usize, offset: usize, flags: usize) -> Result<*mut u8, i32> {
        RegionManager::attach(self, size, offset, flags)
    }
}

pub struct RegionManager;

impl RegionManager {
    pub fn attach(ds: &Dataspace, size: usize, offset: usize, flags: usize) -> Result<*mut u8, i32> {
        let mut addr = core::ptr::null_mut();
        let res = unsafe {
            l4re_rm_attach(&mut addr, size, flags, ds.0 .0, offset, 0)
        };
        if res < 0 {
            Err(res)
        } else {
            Ok(addr as *mut u8)
        }
    }
}

pub struct Goos(pub Cap);

impl Goos {
    pub fn get_info(&self) -> Result<(u32, u32, u8, u32), i32> {
        let mut ginfo = l4re_video_goos_info_t {
            width: 0,
            height: 0,
            flags: 0,
            num_static_views: 0,
            num_static_buffers: 0,
            pixel_info: l4re_video_pixel_info_t {
                r: l4re_video_color_component_t { size: 0, shift: 0 },
                g: l4re_video_color_component_t { size: 0, shift: 0 },
                b: l4re_video_color_component_t { size: 0, shift: 0 },
                a: l4re_video_color_component_t { size: 0, shift: 0 },
                bytes_per_pixel: 0,
            },
        };
        let res = unsafe {
            l4re_video_goos_get_info(self.0 .0, &mut ginfo as *mut _)
        };
        if res < 0 {
            Err(res)
        } else {
            let bpp = ginfo.pixel_info.bytes_per_pixel;
            let pitch = (ginfo.width as u32) * (bpp as u32 / 8);
            Ok((ginfo.width as u32, ginfo.height as u32, bpp, pitch as u32))
        }
    }

    pub fn refresh(&self, x: u32, y: u32, w: u32, h: u32) -> i32 {
        unsafe { l4re_video_goos_refresh(self.0 .0, x, y, w, h) }
    }
}

pub struct Input(pub Cap);

impl Input {
    pub fn get_event(&self) -> Result<l4_input_event_t, i32> {
        // l4re_input_get_event is not available in C API
        // Return error to indicate no event available
        Err(-1)
    }
}

pub fn sleep(ms: u32) {
    unsafe { sys::l4_sleep(ms) };
}

pub fn console_log(message: &[u8]) {
    unsafe {
        sys::puts(message.as_ptr().cast());
    }
}

pub struct IpcServer {
    cap: Cap,
}

impl IpcServer {
    pub fn new(cap: Cap) -> Self {
        Self { cap }
    }

    pub fn run<F>(&self, mut handler: F) -> !
    where
        F: FnMut(&mut l4_utcb_t, l4_msgtag_t, l4_cap_idx_t) -> l4_msgtag_t,
    {
        let mut src = L4_INVALID_CAP;
        let mut tag = l4_msgtag_t { raw: 0 };
        loop {
            tag = ipc_reply_and_wait(tag, &mut src);
            let reply_tag = handler(utcb(), tag, src);
            tag = reply_tag;
        }
    }
}
