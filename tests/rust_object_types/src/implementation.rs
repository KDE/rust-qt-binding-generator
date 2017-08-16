#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use libc::c_int;
use libc::c_uint;
use types::*;
use interface::*;

pub struct Object {
    emit: ObjectEmitter,
    boolean: bool,
    integer: i32,
    uinteger: u32,
    u64: u64,
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
            u64: 0,
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
    fn get_integer(&self) -> i32 {
        self.integer
    }
    fn set_integer(&mut self, value: i32) {
        self.integer = value;
        self.emit.integer_changed();
    }
    fn get_uinteger(&self) -> u32 {
        self.uinteger
    }
    fn set_uinteger(&mut self, value: u32) {
        self.uinteger = value;
        self.emit.uinteger_changed();
    }
    fn get_u64(&self) -> u64 {
        self.u64
    }
    fn set_u64(&mut self, value: u64) {
        self.u64 = value;
        self.emit.u64_changed();
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
