use crate::protocol::SurfaceRole;
use l4_rust::{Cap, utcb, ipc_call, sys::l4_msgtag_t};

#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DisplayRequest {
    GetInfo = 1,
    MapBuffer { slot: u8 } = 2,
    Flip { slot: u8 } = 3,
    Shutdown = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DisplayInfo {
    pub width: u32,
    pub height: u32,
    pub pitch: u32,
    pub bpp: u8,
}

#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CompositorRequest {
    CreateWindow {
        content_width: u32,
        content_height: u32,
        title: [u8; 64],
        role: SurfaceRole,
    } = 1,
    DestroyWindow { window_id: u32 } = 2,
    CommitFrame { window_id: u32 } = 3,
    MoveWindow { window_id: u32, x: i32, y: i32 } = 4,
    ResizeWindow { window_id: u32, width: u32, height: u32 } = 5,
    GetWindowList = 6,
    Shutdown = 7,
}

impl DisplayRequest {
    pub fn dispatch(&self, cap: Cap) -> Result<DisplayInfo, ()> {
        let u = utcb();
        match self {
            DisplayRequest::GetInfo => {
                let tag = l4_msgtag_t::new(1, 0, 0, 0);
                let res = ipc_call(cap, tag);
                if res.label() < 0 { return Err(()); }
                Ok(DisplayInfo {
                    width: u.mr[0] as u32,
                    height: u.mr[1] as u32,
                    pitch: u.mr[2] as u32,
                    bpp: u.mr[3] as u8,
                })
            }
            DisplayRequest::MapBuffer { slot } => {
                u.mr[0] = *slot as usize;
                let tag = l4_msgtag_t::new(2, 1, 0, 0);
                let _res = ipc_call(cap, tag);
                Err(())
            }
            DisplayRequest::Flip { slot } => {
                u.mr[0] = *slot as usize;
                let tag = l4_msgtag_t::new(3, 1, 0, 0);
                let _res = ipc_call(cap, tag);
                Err(())
            }
            DisplayRequest::Shutdown => {
                let tag = l4_msgtag_t::new(4, 0, 0, 0);
                let _res = ipc_call(cap, tag);
                Err(())
            }
        }
    }
}

impl CompositorRequest {
    pub fn dispatch(&self, cap: Cap) -> Result<(), ()> {
        let u = utcb();
        match self {
            CompositorRequest::CreateWindow { content_width, content_height, title, role } => {
                u.mr[0] = *content_width as usize;
                u.mr[1] = *content_height as usize;
                u.mr[2] = *role as usize;
                // Copy title (64 bytes) into MRs (starting from mr[3])
                unsafe {
                    core::ptr::copy_nonoverlapping(
                        title.as_ptr(),
                        u.mr[3..].as_mut_ptr() as *mut u8,
                        64
                    );
                }
                let tag = l4_msgtag_t::new(1, (3 + (64 / core::mem::size_of::<usize>())) as u32, 0, 0);
                let _res = ipc_call(cap, tag);
                Ok(())
            }
            CompositorRequest::DestroyWindow { window_id } => {
                u.mr[0] = *window_id as usize;
                let tag = l4_msgtag_t::new(2, 1, 0, 0);
                let _res = ipc_call(cap, tag);
                Ok(())
            }
            CompositorRequest::CommitFrame { window_id } => {
                u.mr[0] = *window_id as usize;
                let tag = l4_msgtag_t::new(3, 1, 0, 0);
                let _res = ipc_call(cap, tag);
                Ok(())
            }
            CompositorRequest::MoveWindow { window_id, x, y } => {
                u.mr[0] = *window_id as usize;
                u.mr[1] = *x as usize;
                u.mr[2] = *y as usize;
                let tag = l4_msgtag_t::new(4, 3, 0, 0);
                let _res = ipc_call(cap, tag);
                Ok(())
            }
            CompositorRequest::ResizeWindow { window_id, width, height } => {
                u.mr[0] = *window_id as usize;
                u.mr[1] = *width as usize;
                u.mr[2] = *height as usize;
                let tag = l4_msgtag_t::new(5, 3, 0, 0);
                let _res = ipc_call(cap, tag);
                Ok(())
            }
            CompositorRequest::GetWindowList => {
                let tag = l4_msgtag_t::new(6, 0, 0, 0);
                let _res = ipc_call(cap, tag);
                Ok(())
            }
            CompositorRequest::Shutdown => {
                let tag = l4_msgtag_t::new(7, 0, 0, 0);
                let _res = ipc_call(cap, tag);
                Ok(())
            }
        }
    }
}
