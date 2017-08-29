#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

pub struct Object {
    emit: ObjectEmitter,
    boolean: bool,
    bytearray: Vec<u8>,
    integer: i32,
    optional_bytearray: Option<Vec<u8>>,
    optional_string: Option<String>,
    string: String,
    u64: u64,
    uinteger: u32,
}

impl ObjectTrait for Object {
    fn create(emit: ObjectEmitter) -> Object {
        Object {
            emit: emit,
            boolean: true,
            bytearray: Vec::new(),
            integer: 0,
            optional_bytearray: None,
            optional_string: None,
            string: String::new(),
            u64: 0,
            uinteger: 0,
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
    fn get_bytearray(&self) -> Vec<u8> {
        self.bytearray.clone()
    }
    fn set_bytearray(&mut self, value: Vec<u8>) {
        self.bytearray = value;
        self.emit.bytearray_changed();
    }
    fn get_integer(&self) -> i32 {
        self.integer
    }
    fn set_integer(&mut self, value: i32) {
        self.integer = value;
        self.emit.integer_changed();
    }
    fn get_optional_bytearray(&self) -> Option<Vec<u8>> {
        self.optional_bytearray.clone()
    }
    fn set_optional_bytearray(&mut self, value: Option<Vec<u8>>) {
        self.optional_bytearray = value;
        self.emit.optional_bytearray_changed();
    }
    fn get_optional_string(&self) -> Option<String> {
        self.optional_string.clone()
    }
    fn set_optional_string(&mut self, value: Option<String>) {
        self.optional_string = value;
        self.emit.optional_string_changed();
    }
    fn get_string(&self) -> String {
        self.string.clone()
    }
    fn set_string(&mut self, value: String) {
        self.string = value;
        self.emit.string_changed();
    }
    fn get_u64(&self) -> u64 {
        self.u64
    }
    fn set_u64(&mut self, value: u64) {
        self.u64 = value;
        self.emit.u64_changed();
    }
    fn get_uinteger(&self) -> u32 {
        self.uinteger
    }
    fn set_uinteger(&mut self, value: u32) {
        self.uinteger = value;
        self.emit.uinteger_changed();
    }
}

