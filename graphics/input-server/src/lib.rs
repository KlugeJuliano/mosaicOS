#![no_std]
#![no_main]

use l4_rust::{Cap, Input, utcb, ipc_call, sys::l4_msgtag_t};
use mosaic_gui_sdk::input::{InputEvent, MouseButton, Modifiers};

#[no_mangle]
pub extern "C" fn main() {
    let input_cap = Cap::from_env(core::ffi::CStr::from_bytes_with_nul(b"input\0").unwrap()).expect("Could not find input capability");
    let input = Input(input_cap);

    let compositor_cap = Cap::from_env(core::ffi::CStr::from_bytes_with_nul(b"compositor\0").unwrap()).expect("Could not find compositor capability");

    loop {
        if let Ok(ev) = input.get_event() {
            // Translate l4_input_event_t to InputEvent
            // Simplified translation for M5
            let mut gui_ev = None;
            
            match ev.type_ {
                1 => { // Key
                    gui_ev = Some(InputEvent::KeyDown {
                        keycode: ev.code as u32,
                        scancode: ev.code as u32,
                        modifiers: Modifiers::default(),
                    });
                }
                2 => { // Relative Mouse
                    if ev.code == 0 { // DX
                        gui_ev = Some(InputEvent::MouseMove { dx: ev.value, dy: 0 });
                    } else if ev.code == 1 { // DY
                        gui_ev = Some(InputEvent::MouseMove { dx: 0, dy: ev.value });
                    }
                }
                _ => {}
            }

            if let Some(event) = gui_ev {
                // Send to compositor via IPC
                let u = utcb();
                // Serialize event into MRs...
                // This is a simplification
                let tag = l4_msgtag_t::new(0x100, 4, 0, 0); // 0x100 = InputEvent protocol
                ipc_call(compositor_cap, tag);
            }
        }
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
