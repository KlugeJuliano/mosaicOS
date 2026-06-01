#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Modifiers {
    pub shift: bool,
    pub ctrl: bool,
    pub alt: bool,
    pub super_key: bool,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum MouseButton {
    Left,
    Middle,
    Right,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum InputEvent {
    KeyDown { keycode: u32, scancode: u32, modifiers: Modifiers },
    KeyUp { keycode: u32, scancode: u32, modifiers: Modifiers },
    MouseMove { dx: i32, dy: i32 },
    MouseButton { button: MouseButton, pressed: bool },
    MouseScroll { delta: i32 },
}
