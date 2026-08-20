use core::alloc::{GlobalAlloc, Layout};
use core::ptr::null_mut;
use core::sync::atomic::{AtomicUsize, Ordering};

const HEAP_SIZE: usize = 64 * 1024; // 64KB

static mut HEAP: [u8; HEAP_SIZE] = [0; HEAP_SIZE];
static HEAP_TOP: AtomicUsize = AtomicUsize::new(0);

pub struct BumpAllocator;

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let align = layout.align();
        let size = layout.size();
        
        let current_top = HEAP_TOP.load(Ordering::Relaxed);
        let start = (current_top + align - 1) & !(align - 1);
        let end = start + size;
        
        if end > HEAP_SIZE {
            return null_mut();
        }
        
        if HEAP_TOP.compare_exchange_weak(current_top, end, Ordering::Relaxed, Ordering::Relaxed).is_err() {
            return self.alloc(layout);
        }
        
        HEAP.as_mut_ptr().add(start)
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Bump allocator doesn't support deallocation
    }
}

#[global_allocator]
static ALLOCATOR: BumpAllocator = BumpAllocator;

// Re-export alloc crate for convenience
pub use ::alloc::*;