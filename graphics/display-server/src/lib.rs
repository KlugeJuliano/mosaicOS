#![no_std]
#![no_main]

use l4_rust::{Cap, Goos, utcb, ipc_reply_and_wait, sys::l4_msgtag_t};
use mosaic_gui_sdk::ipc::DisplayRequest;

#[no_mangle]
pub extern "C" fn main() {
    let goos_cap = Cap::from_env(core::ffi::CStr::from_bytes_with_nul(b"fb\0").unwrap()).expect("Could not find fb capability");
    let goos = Goos(goos_cap);
    let (width, height, bpp, pitch) = goos.get_info().expect("Could not get goos info");

    let mut src = 0;
    let mut tag = l4_msgtag_t::new(0, 0, 0, 0);

    loop {
        tag = ipc_reply_and_wait(tag, &mut src);
        let u = utcb();
        let label = tag.label();

        match label {
            1 => { // GetInfo
                u.mr[0] = width as usize;
                u.mr[1] = height as usize;
                u.mr[2] = pitch as usize;
                u.mr[3] = bpp as usize;
                tag = l4_msgtag_t::new(0, 4, 0, 0);
            }
            3 => { // Flip
                let _slot = u.mr[0];
                goos.refresh(0, 0, width, height);
                tag = l4_msgtag_t::new(0, 0, 0, 0);
            }
            4 => { // Shutdown
                break;
            }
            _ => {
                tag = l4_msgtag_t::new(-1, 0, 0, 0);
            }
        }
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
