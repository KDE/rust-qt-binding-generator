#![allow(unknown_lints)]
#![allow(mutex_atomic)]
use std::slice;
use libc::{c_int, uint16_t, size_t, c_void};
use types::*;
use std::sync::{Arc, Mutex};
use std::ptr::null;

use implementation::*;

pub struct HelloQObject {}

pub struct HelloEmitter {
    qobject: *const HelloQObject,
    hello_changed: fn(*const HelloQObject),
}

impl HelloEmitter {
    pub fn hello_changed(&self) {
        (self.hello_changed)(self.qobject);
    }
}

pub trait HelloTrait {
    fn create(emit: HelloEmitter) -> Self;
    fn get_hello(&self) -> &String;
    fn set_hello(&mut self, value: String);
}

#[no_mangle]
pub extern "C" fn hello_new(qobject: *const HelloQObject,
                            changed: fn(*const HelloQObject))
                            -> *mut Hello {
    let emit = HelloEmitter {
        qobject: qobject,
        hello_changed: changed,
    };
    let hello = Hello::create(emit);
    Box::into_raw(Box::new(hello))
}

#[no_mangle]
pub unsafe extern "C" fn hello_free(ptr: *mut Hello) {
    if ptr.is_null() {
        return;
    }
    Box::from_raw(ptr);
}

#[no_mangle]
pub unsafe extern "C" fn hello_set(ptr: *mut Hello, s: *const uint16_t, len: size_t) {
    let data = slice::from_raw_parts(s, len as usize);
    (&mut *ptr).set_hello(String::from_utf16_lossy(data));
}

#[no_mangle]
pub unsafe extern "C" fn hello_get(ptr: *const Hello) -> QString {
    QString::from((&*ptr).get_hello())
}


pub struct RItemModelQObject {}

#[derive (Clone)]
pub struct RItemModelEmitter {
    qobject: Arc<Mutex<*const RItemModelQObject>>,
    new_data_ready: fn(*const RItemModelQObject),
}

unsafe impl Send for RItemModelEmitter {}

impl RItemModelEmitter {
    pub fn new_data_ready(&self) {
        let ptr = *self.qobject.lock().unwrap();
        if !ptr.is_null() {
            (self.new_data_ready)(ptr);
        }
    }
    fn clear(&self) {
        *self.qobject.lock().unwrap() = null();
    }
}

pub trait RItemModelTrait<T> {
    fn create(emit: RItemModelEmitter, root: T) -> Self;
    fn emit(&self) -> &RItemModelEmitter;
    fn row_count(&mut self, row: c_int, parent: usize) -> c_int;
    fn index(&mut self, row: c_int, parent: usize) -> usize;
    fn parent(&self, row: c_int, parent: usize) -> QModelIndex;
    fn file_name(&mut self, row: c_int, parent: usize) -> String;
    fn file_permissions(&mut self, row: c_int, parent: usize) -> c_int;
}

#[no_mangle]
pub extern "C" fn ritemmodel_new(qobject: *const RItemModelQObject,
                                 new_data_ready: fn(*const RItemModelQObject))
                                 -> *mut RItemModel {
    let emit = RItemModelEmitter {
        qobject: Arc::new(Mutex::new(qobject)),
        new_data_ready: new_data_ready,
    };
    let ritemmodel = RItemModel::create(emit, DirEntry::create("/"));
    Box::into_raw(Box::new(ritemmodel))
}

#[no_mangle]
pub unsafe extern "C" fn ritemmodel_free(ptr: *mut RItemModel) {
    Box::from_raw(ptr).emit().clear();
}

#[no_mangle]
pub unsafe extern "C" fn ritemmodel_row_count(ptr: *mut RItemModel, row: c_int, parent: usize) -> i32 {
    (&mut *ptr).row_count(row, parent)
}

#[no_mangle]
pub unsafe extern "C" fn ritemmodel_index(ptr: *mut RItemModel,
                                          row: c_int, parent: usize)
                                          -> usize {
    (&mut *ptr).index(row, parent)
}

#[no_mangle]
pub unsafe extern "C" fn ritemmodel_parent(ptr: *const RItemModel,
                                           row: c_int, parent: usize)
                                           -> QModelIndex {
    (&*ptr).parent(row, parent)
}

#[no_mangle]
pub unsafe extern "C" fn ritemmodel_data_file_name(ptr: *mut RItemModel,
                                    row: c_int, parent: usize,
        d: *mut c_void,
        set: fn(*mut c_void, QString)) {
    let data = (&mut *ptr).file_name(row, parent);
    set(d, QString::from(&data));
}

#[no_mangle]
pub unsafe extern "C" fn ritemmodel_data_file_permissions(ptr: *mut RItemModel,
                                    row: c_int, parent: usize) -> c_int {
    (&mut *ptr).file_permissions(row, parent)
}
