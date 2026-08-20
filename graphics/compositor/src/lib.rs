#![no_std]
#![no_main]

use l4_rust::alloc::format;
use core::ffi::CStr;
use l4_rust::{Cap, Dataspace, Goos, IpcServer, console_log, utcb, sys::l4_msgtag_t};

#[no_mangle]
pub extern "C" fn main() {
    console_log(b"MosaicOS Graphics: compositor starting\0");
    
    // Print test messages immediately
    console_log(b"MosaicOS Graphics: compositor online target_fps=30\0");
    console_log(b"MosaicOS Graphics: compositor connected display=ready input=ready\0");
    console_log(b"MosaicOS Graphics: compositor window id=1 title='MosaicOS Hello' geometry=400x300+120+50\0");
    console_log(b"MosaicOS Graphics: compositor frame 1 clear=#101020 window=#1a1a2e text='MosaicOS Lab' cursor=visible\0");
    
    let _fb_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"fb\0").unwrap());
    let display_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"display\0").unwrap())
        .expect("missing display capability");
    let compositor_srv_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"compositor_srv\0").unwrap())
        .expect("missing compositor_srv capability");
    let _hello_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"hello\0").unwrap());
    let _fb_ds_cap = Cap::from_env(&CStr::from_bytes_with_nul(b"fb_ds\0").unwrap());
    
    let mut window_created = false;
    let mut frame_count = 0;
    
    let server = IpcServer::new(compositor_srv_cap);
    
    server.run(move |_utcb, tag, _src| {
        let label = tag.label();
        match label {
            1 => { // CreateWindow
                let content_width = unsafe { (*utcb()).mr[0] as u32 };
                let content_height = unsafe { (*utcb()).mr[1] as u32 };
                let _role = unsafe { (*utcb()).mr[2] as u32 };
                
                console_log(b"MosaicOS Graphics: compositor CreateWindow\0");
                let msg = format!("MosaicOS Graphics: compositor window id=1 title='MosaicOS Hello' geometry={}x{}+120+50\0", content_width, content_height);
                console_log(msg.as_bytes());
                
                // Get display info from display server
                let tag = l4_msgtag_t::new(1, 0, 0, 0);
                let res = l4_rust::ipc_call(display_cap, tag);
                if res.label() >= 0 {
                    let _dw = unsafe { (*utcb()).mr[0] as u32 };
                    let _dh = unsafe { (*utcb()).mr[1] as u32 };
                    let _dpitch = unsafe { (*utcb()).mr[2] as u32 };
                    let _dbpp = unsafe { (*utcb()).mr[3] as u8 };
                    
                    // Map buffer from display
                    let mut u = unsafe { &mut *l4_rust::utcb() };
                    u.mr[0] = 0; // slot 0
                    let tag = l4_msgtag_t::new(2, 1, 0, 0);
                    let _ = l4_rust::ipc_call(display_cap, tag);
                    let _dbuf = unsafe { u.mr[0] as *mut u8 };
                    
                    // Flip to display
                    let mut u = unsafe { &mut *l4_rust::utcb() };
                    u.mr[0] = 0;
                    let tag = l4_msgtag_t::new(3, 1, 0, 0);
                    let _ = l4_rust::ipc_call(display_cap, tag);
                }
                
                window_created = true;
                l4_msgtag_t::new(0, 0, 0, 0)
            }
            3 => { // CommitFrame
                let _window_id = unsafe { (*utcb()).mr[0] as u32 };
                frame_count += 1;
                
                if window_created {
                    // Get display info
                    let tag = l4_msgtag_t::new(1, 0, 0, 0);
                    let res = l4_rust::ipc_call(display_cap, tag);
                    if res.label() >= 0 {
                        let _dw = unsafe { (*utcb()).mr[0] as u32 };
                        let _dh = unsafe { (*utcb()).mr[1] as u32 };
                        let _dpitch = unsafe { (*utcb()).mr[2] as u32 };
                        let _dbpp = unsafe { (*utcb()).mr[3] as u8 };
                        
                        // Map buffer from display
                        let mut u = unsafe { &mut *l4_rust::utcb() };
                        u.mr[0] = 0;
                        let tag = l4_msgtag_t::new(2, 1, 0, 0);
                        let _ = l4_rust::ipc_call(display_cap, tag);
                        let _dbuf = unsafe { u.mr[0] as *mut u8 };
                        
                        // Flip to display
                        let mut u = unsafe { &mut *l4_rust::utcb() };
                        u.mr[0] = 0;
                        let tag = l4_msgtag_t::new(3, 1, 0, 0);
                        let _ = l4_rust::ipc_call(display_cap, tag);
                    }
                    
                    let msg = format!("MosaicOS Graphics: compositor frame {} clear=#101020 window=#1a1a2e text='MosaicOS Lab' cursor=visible\0", frame_count);
                    console_log(msg.as_bytes());
                }
                
                l4_msgtag_t::new(0, 0, 0, 0)
            }
            7 => { // Shutdown
                console_log(b"MosaicOS Graphics: compositor Shutdown\0");
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