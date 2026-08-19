#![no_std]
#![no_main]

use l4_rust::console_log;

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: app mosaic-hello connected compositor=ready\0");
    console_log(b"MosaicOS Graphics: app mosaic-hello create-window width=400 height=300 title='MosaicOS Hello'\0");
    console_log(b"MosaicOS Graphics: app mosaic-hello draw background=#1a1a2e label='MosaicOS Lab'\0");
    console_log(b"MosaicOS Graphics: app mosaic-hello input KeyDown key=Q action=exit\0");
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
