#![allow(unused_imports)]
use libc::c_int;
use libc::c_uint;
use types::*;
use interface::*;

pub struct Object {
    emit: ObjectEmitter,
    boolean: bool,
    integer: c_int,
    uinteger: c_uint,
    string: String,
    bytearray: Vec<u8>,
}

impl ObjectTrait for Object {
    fn create(emit: ObjectEmitter) -> Object {
        Object {
            emit: emit,
            boolean: true,
            integer: 0,
            uinteger: 0,
            string: String::new(),
            bytearray: Vec::new(),
        }
    }
    fn emit(&self) -> &ObjectEmitter {
        &self.emit
    }
    fn get_boolean(&self) -> bool {
        self.boolean
    }
    fn set_boolean(&mut self, value: bool) {
        self.boolean = value;
        self.emit.boolean_changed();
    }
    fn get_integer(&self) -> c_int {
        self.integer
    }
    fn set_integer(&mut self, value: c_int) {
        self.integer = value;
        self.emit.integer_changed();
    }
    fn get_uinteger(&self) -> c_uint {
        self.uinteger
    }
    fn set_uinteger(&mut self, value: c_uint) {
        self.uinteger = value;
        self.emit.uinteger_changed();
    }
    fn get_string(&self) -> String {
        self.string.clone()
    }
    fn set_string(&mut self, value: String) {
        self.string = value;
        self.emit.string_changed();
    }
    fn get_bytearray(&self) -> Vec<u8> {
        self.bytearray.clone()
    }
    fn set_bytearray(&mut self, value: Vec<u8>) {
        self.bytearray = value;
        self.emit.bytearray_changed();
    }
}
