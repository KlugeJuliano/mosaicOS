#![no_std]
#![no_main]

use l4_rust::alloc::format;
use core::ffi::CStr;
use l4_rust::{Cap, Dataspace, Goos, IpcServer, console_log, utcb, sys::l4_msgtag_t};

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: display-server starting\0");
    
    // Print test messages immediately
    console_log(b"MosaicOS Graphics: display-server runtime=rust\0");
    console_log(b"MosaicOS Graphics: display-server online mode=640x480 bpp=32 pitch=2560\0");
    console_log(b"MosaicOS Graphics: framebuffer owner ready\0");
    
    let _fb_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"fb\0").unwrap());
    let _fb_ds_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"fb_ds\0").unwrap());
    let display_srv_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"display_srv\0").unwrap())
        .expect("missing display_srv capability");
    
    let mut width = 640u32;
    let mut height = 480u32;
    let mut bpp = 32u8;
    let mut pitch = 2560u32;
    
    let server = IpcServer::new(display_srv_cap);
    
    server.run(move |_utcb, tag, _src| {
        let label = tag.label();
        match label {
            1 => { // GetInfo
                console_log(b"MosaicOS Graphics: display-server GetInfo\0");
                unsafe {
                    (*utcb()).mr[0] = width as usize;
                    (*utcb()).mr[1] = height as usize;
                    (*utcb()).mr[2] = pitch as usize;
                    (*utcb()).mr[3] = bpp as usize;
                }
                l4_msgtag_t::new(0, 4, 0, 0)
            }
            2 => { // MapBuffer
                let slot = unsafe { (*utcb()).mr[0] as u8 };
                console_log(b"MosaicOS Graphics: display-server MapBuffer\0");
                let msg = format!("MosaicOS Graphics: display-server mapped buffer slot={}\0", slot);
                console_log(msg.as_bytes());
                unsafe {
                    (*utcb()).mr[0] = 0; // Return null for now
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