use std::slice;
use libc::{uint8_t, uint16_t, size_t};

use implementation::Hello;

pub struct HelloQObject {}

pub struct HelloNotifier {
    qobject: *const HelloQObject,
    hello_changed: fn (*const HelloQObject)
}

impl HelloNotifier {
    pub fn hello_changed(&self) {
        (self.hello_changed)(self.qobject);
    }
}

pub trait HelloTrait {
    fn create(notifier: HelloNotifier) -> Self;
    fn get_hello(&self) -> &String;
    fn set_hello(&mut self, value: String);
}

#[no_mangle]
pub extern fn hello_new(qobject: *const HelloQObject, changed: fn(*const HelloQObject)) -> *mut Hello {
    let notifier = HelloNotifier {
        qobject: qobject,
        hello_changed: changed
    };
    let hello = Hello::create(notifier);
    Box::into_raw(Box::new(hello))
}

#[no_mangle]
pub extern fn hello_set(ptr: *mut Hello, s: *const uint16_t, len: size_t) {
    let (hello, data) = unsafe {
        (&mut *ptr, slice::from_raw_parts(s, len as usize))
    };
    hello.set_hello(String::from_utf16_lossy(data));
}

#[no_mangle]
pub extern fn hello_size(ptr: *mut Hello) -> size_t {
    let hello = unsafe {
        &mut *ptr
    };
    hello.get_hello().len()
}

#[no_mangle]
pub extern fn hello_get(ptr: *mut Hello) -> *const uint8_t {
    let hello = unsafe {
        &mut *ptr
    };
    hello.get_hello().as_ptr()
}

#[no_mangle]
pub extern fn hello_free(ptr: *mut Hello) {
    if ptr.is_null() { return }
    unsafe { Box::from_raw(ptr); }
}
