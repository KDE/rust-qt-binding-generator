use std::slice;
use libc::{c_int, uint16_t, size_t, c_void};
use types::*;
use std::sync::{Arc, Mutex};
use std::ptr::null;

use implementation::*;

pub struct HelloQObject {}

pub struct HelloEmitter {
    qobject: *const HelloQObject,
    hello_changed: fn (*const HelloQObject)
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
pub extern fn hello_new(qobject: *const HelloQObject, changed: fn(*const HelloQObject)) -> *mut Hello {
    let emit = HelloEmitter {
        qobject: qobject,
        hello_changed: changed
    };
    let hello = Hello::create(emit);
    Box::into_raw(Box::new(hello))
}

#[no_mangle]
pub extern fn hello_free(ptr: *mut Hello) {
    if ptr.is_null() { return }
    unsafe { Box::from_raw(ptr); }
}

#[no_mangle]
pub extern fn hello_set(ptr: *mut Hello, s: *const uint16_t, len: size_t) {
    let (hello, data) = unsafe {
        (&mut *ptr, slice::from_raw_parts(s, len as usize))
    };
    hello.set_hello(String::from_utf16_lossy(data));
}

#[no_mangle]
pub extern fn hello_get(ptr: *mut Hello) -> QString {
    let hello = unsafe { &mut *ptr };
    QString::from(hello.get_hello())
}


pub struct RItemModelQObject {}

#[derive (Clone)]
pub struct RItemModelEmitter {
    qobject: Arc<Mutex<*const RItemModelQObject>>,
    new_data_ready: fn (*const RItemModelQObject)
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
    fn column_count(&mut self, parent: QModelIndex) -> c_int;
    fn row_count(&mut self, parent: QModelIndex) -> c_int;
    fn index(&mut self, row: c_int, column: c_int, parent: QModelIndex) -> QModelIndex;
    fn parent(&self, index: QModelIndex) -> QModelIndex;
    fn data<'a>(&'a mut self, index: QModelIndex, role: c_int) -> Variant<'a>;
}

#[no_mangle]
pub extern fn ritemmodel_new(qobject: *const RItemModelQObject, new_data_ready: fn(*const RItemModelQObject)) -> *mut RItemModel {
    let emit = RItemModelEmitter {
        qobject: Arc::new(Mutex::new(qobject)),
        new_data_ready: new_data_ready
    };
    let ritemmodel = RItemModel::create(emit, DirEntry::create("/"));
    Box::into_raw(Box::new(ritemmodel))
}

#[no_mangle]
pub extern fn ritemmodel_free(ptr: *mut RItemModel) {
    let ritemmodel = unsafe { Box::from_raw(ptr) };
    ritemmodel.emit().clear();
}

#[no_mangle]
pub extern fn ritemmodel_column_count(ptr: *mut RItemModel, parent: QModelIndex) -> i32 {
    let ritemmodel = unsafe { &mut *ptr };
    ritemmodel.column_count(parent)
}

#[no_mangle]
pub extern fn ritemmodel_row_count(ptr: *mut RItemModel, parent: QModelIndex) -> i32 {
    let ritemmodel = unsafe { &mut *ptr };
    ritemmodel.row_count(parent)
}

#[no_mangle]
pub extern fn ritemmodel_index(ptr: *mut RItemModel, row: i32, column: i32, parent: QModelIndex) -> QModelIndex {
    let ritemmodel = unsafe { &mut *ptr };
    ritemmodel.index(row, column, parent)
}

#[no_mangle]
pub extern fn ritemmodel_parent(ptr: *const RItemModel, index: QModelIndex) -> QModelIndex {
    let ritemmodel = unsafe { &*ptr };
    ritemmodel.parent(index)
}

#[no_mangle]
pub extern fn ritemmodel_data(ptr: *mut RItemModel, index: QModelIndex, role: c_int, d: *mut c_void, set: fn (*mut c_void, &QVariant)) {
    let ritemmodel = unsafe { &mut *ptr };
    let data = ritemmodel.data(index, role);
    set(d, &QVariant::from(&data));
}
