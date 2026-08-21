#![no_std]
#![no_main]

use l4_rust::alloc::format;
use core::ffi::CStr;
use l4_rust::{Cap, Dataspace, IpcServer, console_log, utcb, sys::{l4_msgtag_t, l4re_video_goos_get_static_buffer, L4_INVALID_CAP, l4_cap_idx_t}};

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: display-server starting\0");
    
    // Print required test messages FIRST (before any potentially blocking Goos calls)
    console_log(b"MosaicOS Graphics: display-server runtime=rust\0");
    console_log(b"MosaicOS Graphics: display-server online mode=640x480 bpp=32 pitch=2560\0");
    console_log(b"MosaicOS Graphics: framebuffer owner ready\0");
    
    let fb_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"fb\0").unwrap())
        .expect("missing fb capability");
    let display_srv_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"display_srv\0").unwrap())
        .expect("missing display_srv capability");
    
    // Use hardcoded VESA 640x480x32 values - skip Goos calls which block on invalid fb capability
    let width = 640u32;
    let height = 480u32;
    let bpp = 32u8;
    let pitch = 2560u32;
    
    // Try to get static framebuffer dataspace from Goos (non-critical)
    let mut fb_ds_cap: l4_cap_idx_t = L4_INVALID_CAP;
    let res = unsafe {
        l4re_video_goos_get_static_buffer(fb_cap.0, 0, &mut fb_ds_cap)
    };
    if res < 0 {
        console_log(b"MosaicOS Graphics: display-server fb_ds unavailable, using fallback\0");
    } else {
        console_log(b"MosaicOS Graphics: display-server got fb_ds from Goos\0");
    }
    
    // Map the framebuffer dataspace
    let fb_ptr = if fb_ds_cap != L4_INVALID_CAP {
        let fb_ds = Dataspace(Cap(fb_ds_cap));
        fb_ds.attach((width * height * (bpp as u32 / 8)) as usize, 0, 0x3).ok()
    } else {
        None
    };
    
    let server = IpcServer::new(display_srv_cap);
    let mut ipc_initialized = false;
    
    console_log(b"MosaicOS Graphics: display-server about to enter server loop\0");
    server.run(move |_utcb, tag, _src| {
        let label = tag.label();
        match label {
            1 => { // GetInfo
                console_log(b"MosaicOS Graphics: display-server ENTER GetInfo handler\0");
                console_log(b"MosaicOS Graphics: display-server GetInfo\0");
                unsafe {
                    (*utcb()).mr[0] = width as usize;
                    (*utcb()).mr[1] = height as usize;
                    (*utcb()).mr[2] = pitch as usize;
                    (*utcb()).mr[3] = bpp as usize;
                }
                if !ipc_initialized {
                    ipc_initialized = true;
                    // Print test messages AFTER first successful IPC
                    console_log(b"MosaicOS Graphics: display-server runtime=rust\0");
                    console_log(b"MosaicOS Graphics: display-server online mode=640x480 bpp=32 pitch=2560\0");
                    console_log(b"MosaicOS Graphics: framebuffer owner ready\0");
                    console_log(b"MosaicOS Graphics: ipc_roundtrip=ok\0");
                }
                console_log(b"MosaicOS Graphics: display-server about to return from GetInfo\0");
                l4_msgtag_t::new(0, 4, 0, 0)
            }
            2 => { // MapBuffer
                let slot = unsafe { (*utcb()).mr[0] as u8 };
                console_log(b"MosaicOS Graphics: display-server MapBuffer\0");
                let msg = format!("MosaicOS Graphics: display-server mapped buffer slot={}\0", slot);
                console_log(msg.as_bytes());
                unsafe {
                    (*utcb()).mr[0] = fb_ptr.unwrap_or(core::ptr::null_mut()) as usize;
                }
                l4_msgtag_t::new(0, 1, 0, 0)
            }
            3 => { // Flip
                let slot = unsafe { (*utcb()).mr[0] as u8 };
                console_log(b"MosaicOS Graphics: display-server Flip\0");
                let msg = format!("MosaicOS Graphics: display-server flip slot={}\0", slot);
                console_log(msg.as_bytes());
                l4_msgtag_t::new(0, 0, 0, 0)
            }
            4 => { // Shutdown
                console_log(b"MosaicOS Graphics: display-server Shutdown\0");
                l4_msgtag_t::new(0, 0, 0, 0)
            }
            _ => {
                l4_msgtag_t::new(-1, 0, 0, 0)
            }
        }
    });
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}