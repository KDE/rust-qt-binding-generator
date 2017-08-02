extern crate libc;

use std::slice;
use std::ffi::CString;
use std::ffi::CStr;
use libc::{uint8_t, c_char, uint32_t, uint16_t, size_t};

pub struct Hello {
    hello: String
}

#[no_mangle]
pub extern fn add(lhs: u32, rhs: u32) -> u32 {
   lhs + rhs
}

#[no_mangle]
pub extern fn hello_new() -> *mut Hello {
    let s = String::from("hello");
    Box::into_raw(Box::new(Hello { hello: s }))
}

#[no_mangle]
pub extern fn hello_set(ptr: *mut Hello, s: *const uint16_t, len: size_t) {
    let (hello, data) = unsafe {
        (&mut *ptr, slice::from_raw_parts(s, len as usize))
    };
    hello.hello = String::from_utf16_lossy(data);
}

#[no_mangle]
pub extern fn hello_size(ptr: *mut Hello) -> size_t {
    let hello = unsafe {
        &mut *ptr
    };
    hello.hello.len()
}

#[no_mangle]
pub extern fn hello_get(ptr: *mut Hello) -> *const uint8_t {
    let hello = unsafe {
        &mut *ptr
    };
    hello.hello.as_ptr()
}

#[no_mangle]
pub extern fn hello_free(ptr: *mut Hello) {
    if ptr.is_null() { return }
    unsafe { Box::from_raw(ptr); }
}
