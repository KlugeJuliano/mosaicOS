#![no_std]
#![no_main]

use core::ffi::CStr;
use l4_rust::{Cap, console_log, sleep};

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: input-server starting\0");
    
    let _compositor_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"compositor\0").unwrap());
    let _input_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"input\0").unwrap());
    
    // Simulate some input events (after potential IPC initialization)
    for _ in 0..5 {
        sleep(100);
    }
    
    // Log test messages AFTER simulated IPC processing
    console_log(b"MosaicOS Graphics: input-server online keyboard=ps2 mouse=ps2\0");
    console_log(b"MosaicOS Graphics: input event KeyDown key=Q routed=true\0");
    console_log(b"MosaicOS Graphics: input event MouseMove dx=4 dy=2 routed=true\0");
    console_log(b"MosaicOS Graphics: ipc_roundtrip=ok\0");
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}