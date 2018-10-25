/* generated by rust_qt_binding_generator */
use libc::{c_char, c_ushort, c_int};
use std::slice;
use std::char::decode_utf16;

use std::sync::Arc;
use std::sync::atomic::{AtomicPtr, Ordering};
use std::ptr::null;

use implementation::*;


pub enum QString {}

fn set_string_from_utf16(s: &mut String, str: *const c_ushort, len: c_int) {
    let utf16 = unsafe { slice::from_raw_parts(str, to_usize(len)) };
    let characters = decode_utf16(utf16.iter().cloned())
        .map(|r| r.unwrap());
    s.clear();
    s.extend(characters);
}



fn to_usize(n: c_int) -> usize {
    if n < 0 {
        panic!("Cannot cast {} to usize", n);
    }
    n as usize
}


fn to_c_int(n: usize) -> c_int {
    if n > c_int::max_value() as usize {
        panic!("Cannot cast {} to c_int", n);
    }
    n as c_int
}


pub struct SimpleQObject {}

pub struct SimpleEmitter {
    qobject: Arc<AtomicPtr<SimpleQObject>>,
    message_changed: fn(*mut SimpleQObject),
}

unsafe impl Send for SimpleEmitter {}

impl SimpleEmitter {
    /// Clone the emitter
    ///
    /// The emitter can only be cloned when it is mutable. The emitter calls
    /// into C++ code which may call into Rust again. If emmitting is possible
    /// from immutable structures, that might lead to access to a mutable
    /// reference. That is undefined behaviour and forbidden.
    pub fn clone(&mut self) -> SimpleEmitter {
        SimpleEmitter {
            qobject: self.qobject.clone(),
            message_changed: self.message_changed,
        }
    }
    fn clear(&self) {
        let n: *const SimpleQObject = null();
        self.qobject.store(n as *mut SimpleQObject, Ordering::SeqCst);
    }
    pub fn message_changed(&mut self) {
        let ptr = self.qobject.load(Ordering::SeqCst);
        if !ptr.is_null() {
            (self.message_changed)(ptr);
        }
    }
}

pub trait SimpleTrait {
    fn new(emit: SimpleEmitter) -> Self;
    fn emit(&self) -> &SimpleEmitter;
    fn message(&self) -> &str;
    fn set_message(&mut self, value: String);
}

#[no_mangle]
pub extern "C" fn simple_new(
    simple: *mut SimpleQObject,
    simple_message_changed: fn(*mut SimpleQObject),
) -> *mut Simple {
    let simple_emit = SimpleEmitter {
        qobject: Arc::new(AtomicPtr::new(simple)),
        message_changed: simple_message_changed,
    };
    let d_simple = Simple::new(simple_emit);
    Box::into_raw(Box::new(d_simple))
}

#[no_mangle]
pub unsafe extern "C" fn simple_free(ptr: *mut Simple) {
    Box::from_raw(ptr).emit().clear();
}

#[no_mangle]
pub unsafe extern "C" fn simple_message_get(
    ptr: *const Simple,
    p: *mut QString,
    set: fn(*mut QString, *const c_char, c_int),
) {
    let o = &*ptr;
    let v = o.message();
    let s: *const c_char = v.as_ptr() as (*const c_char);
    set(p, s, to_c_int(v.len()));
}

#[no_mangle]
pub unsafe extern "C" fn simple_message_set(ptr: *mut Simple, v: *const c_ushort, len: c_int) {
    let o = &mut *ptr;
    let mut s = String::new();
    set_string_from_utf16(&mut s, v, len);
    o.set_message(s);
}
