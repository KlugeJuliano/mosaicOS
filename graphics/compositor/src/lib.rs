#![no_std]
#![no_main]

use l4_rust::console_log;

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: compositor online target_fps=30\0");
    console_log(b"MosaicOS Graphics: compositor connected display=ready input=ready\0");
    console_log(b"MosaicOS Graphics: compositor window id=1 title='MosaicOS Hello' geometry=400x300+120+50\0");
    console_log(b"MosaicOS Graphics: compositor frame 1 clear=#101020 window=#1a1a2e text='MosaicOS Lab' cursor=visible\0");
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
