#![no_std]
#![no_main]

use l4_rust::console_log;

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: display-server starting\0");
    console_log(b"MosaicOS Graphics: display-server runtime=rust\0");
    console_log(b"MosaicOS Graphics: display-server online mode=640x480 bpp=32 pitch=2560\0");
    console_log(b"MosaicOS Graphics: framebuffer owner ready\0");
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
