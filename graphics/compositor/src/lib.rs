#![no_std]
#![no_main]

use l4_rust::{Cap, utcb, ipc_reply_and_wait, ipc_call, sleep, sys::l4_msgtag_t};
use mosaic_gui_sdk::ipc::{DisplayRequest, DisplayInfo};
use mosaic_gui_sdk::font::{draw_string, GLYPH_WIDTH, GLYPH_HEIGHT};

struct Window {
    id: u32,
    x: i32,
    y: i32,
    width: u32,
    height: u32,
    title: [u8; 64],
}

#[no_mangle]
pub extern "C" fn main() {
    let display_cap = Cap::from_env(core::ffi::CStr::from_bytes_with_nul(b"display\0").unwrap()).expect("Could not find display capability");
    
    // Get display info
    let info = DisplayRequest::GetInfo.dispatch(display_cap).expect("Could not get display info");
    
    // Map display buffer
    // For M5, we assume we can write to the buffer.
    // In a real system, we'd use MapBuffer to get the cap and then map it.
    let display_pixels: &mut [u32] = unsafe {
        core::slice::from_raw_parts_mut(0xdeadbeef as *mut u32, (info.pitch * info.height / 4) as usize)
    };

    let mut src = 0;
    let mut tag = l4_msgtag_t::new(0, 0, 0, 0);

    loop {
        // Composition loop
        // 1. Clear background
        display_pixels.fill(0xFF0D0D0D);

        // 2. Draw windows (simulated for now)
        draw_string(display_pixels, info.pitch / 4, 10, 10, b"MosaicOS Compositor", 0xFFFFFFFF, 0, true);

        // 3. Flip
        DisplayRequest::Flip { slot: 0 }.dispatch(display_cap).ok();

        // 4. Wait for IPC or next frame
        // This is a placeholder for real IPC + timing
        tag = ipc_reply_and_wait(tag, &mut src);
        
        // Handle IPC...
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
