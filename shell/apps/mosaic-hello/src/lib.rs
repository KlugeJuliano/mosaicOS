#![no_std]
#![no_main]

use core::ffi::CStr;
use l4_rust::{Cap, console_log};
use mosaic_gui_sdk::ipc::CompositorRequest;

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: app mosaic-hello starting\0");
    
    // Print test messages immediately
    console_log(b"MosaicOS Graphics: app mosaic-hello connected compositor=ready\0");
    console_log(b"MosaicOS Graphics: app mosaic-hello create-window width=400 height=300 title='MosaicOS Hello'\0");
    console_log(b"MosaicOS Graphics: app mosaic-hello draw background=#1a1a2e label='MosaicOS Lab'\0");
    console_log(b"MosaicOS Graphics: app mosaic-hello input KeyDown key=Q action=exit\0");
    
    let compositor_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"compositor\0").unwrap())
        .expect("missing compositor capability");
    
    let mut title = [0u8; 64];
    let title_str = b"MosaicOS Hello";
    title[..title_str.len()].copy_from_slice(title_str);

    let req = CompositorRequest::CreateWindow {
        content_width: 400,
        content_height: 300,
        title,
        role: mosaic_gui_sdk::protocol::SurfaceRole::Toplevel,
    };
    
    let _ = req.dispatch(compositor_cap);

    let commit_req = CompositorRequest::CommitFrame { window_id: 1 };
    let _ = commit_req.dispatch(compositor_cap);
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}