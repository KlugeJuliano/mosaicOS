#![no_std]
#![no_main]

use l4_rust::{Cap, utcb, ipc_call, sys::l4_msgtag_t};
use mosaic_gui_sdk::ipc::CompositorRequest;
use mosaic_gui_sdk::protocol::SurfaceRole;
use mosaic_gui_sdk::font::draw_string;

#[no_mangle]
pub extern "C" fn main() {
    let compositor_cap = Cap::from_env(core::ffi::CStr::from_bytes_with_nul(b"compositor\0").unwrap()).expect("Could not find compositor capability");

    let mut title = [0u8; 64];
    let title_str = b"MosaicOS Hello";
    title[..title_str.len()].copy_from_slice(title_str);

    // Request window
    CompositorRequest::CreateWindow {
        content_width: 400,
        content_height: 300,
        title,
        role: SurfaceRole::Toplevel,
    }.dispatch(compositor_cap).expect("Could not create window");

    // Main loop
    loop {
        // Draw into surface (once we have shared memory working)
        
        // Commit frame
        CompositorRequest::CommitFrame { window_id: 1 }.dispatch(compositor_cap).ok();
        
        // Wait for events...
        l4_rust::sleep(100);
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
