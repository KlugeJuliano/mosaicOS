#![no_std]
#![no_main]

use l4_rust::console_log;

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: input-server online keyboard=ps2 mouse=ps2\0");
    console_log(b"MosaicOS Graphics: input event KeyDown key=Q routed=false\0");
    console_log(b"MosaicOS Graphics: input event MouseMove dx=4 dy=2 routed=false\0");
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
