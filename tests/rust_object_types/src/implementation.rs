#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

pub struct Object {
    emit: ObjectEmitter,
    boolean: bool,
    optional_boolean: Option<bool>,
    i8: i8,
    u8: u8,
    i16: i16,
    u16: u16,
    i32: i32,
    u32: u32,
    i64: i64,
    u64: u64,
    optional_u64: Option<u64>,
    f32: f32,
    f64: f64,
    bytearray: Vec<u8>,
    optional_bytearray: Option<Vec<u8>>,
    string: String,
    optional_string: Option<String>,
}

impl ObjectTrait for Object {
    fn new(emit: ObjectEmitter) -> Object {
        Object {
            emit,
            boolean: false,
            optional_boolean: None,
            i8: 0,
            u8: 0,
            i16: 0,
            u16: 0,
            i32: 0,
            u32: 0,
            i64: 0,
            u64: 0,
            optional_u64: None,
            f32: 0.,
            f64: 0.,
            bytearray: Vec::new(),
            optional_bytearray: None,
            string: String::new(),
            optional_string: None
        }
    }
    fn emit(&self) -> &ObjectEmitter {
        &self.emit
    }
    fn boolean(&self) -> bool {
        self.boolean
    }
    fn set_boolean(&mut self, value: bool) {
        self.boolean = value;
        self.emit.boolean_changed();
    }
    fn optional_boolean(&self) -> Option<bool> {
        self.optional_boolean
    }
    fn set_optional_boolean(&mut self, b: Option<bool>) {
        self.optional_boolean = b;
        self.emit.optional_boolean_changed();
    }
    fn i8(&self) -> i8 {
        self.i8
    }
    fn set_i8(&mut self, value: i8) {
        self.i8 = value;
        self.emit.i8_changed();
    }
    fn u8(&self) -> u8 {
        self.u8
    }
    fn set_u8(&mut self, value: u8) {
        self.u8 = value;
        self.emit.u8_changed();
    }
    fn i16(&self) -> i16 {
        self.i16
    }
    fn set_i16(&mut self, value: i16) {
        self.i16 = value;
        self.emit.i16_changed();
    }
    fn u16(&self) -> u16 {
        self.u16
    }
    fn set_u16(&mut self, value: u16) {
        self.u16 = value;
        self.emit.u16_changed();
    }
    fn i32(&self) -> i32 {
        self.i32
    }
    fn set_i32(&mut self, value: i32) {
        self.i32 = value;
        self.emit.i32_changed();
    }
    fn u32(&self) -> u32 {
        self.u32
    }
    fn set_u32(&mut self, value: u32) {
        self.u32 = value;
        self.emit.u32_changed();
    }
    fn i64(&self) -> i64 {
        self.i64
    }
    fn set_i64(&mut self, value: i64) {
        self.i64 = value;
        self.emit.i64_changed();
    }
    fn u64(&self) -> u64 {
        self.u64
    }
    fn set_u64(&mut self, value: u64) {
        self.u64 = value;
        self.emit.u64_changed();
    }
    fn optional_u64(&self) -> Option<u64> {
        self.optional_u64
    }
    fn set_optional_u64(&mut self, v: Option<u64>) {
        self.optional_u64 = v;
        self.emit.optional_u64_changed();
    }
    fn f32(&self) -> f32 {
        self.f32
    }
    fn set_f32(&mut self, value: f32) {
        self.f32 = value;
        self.emit.f32_changed();
    }
    fn f64(&self) -> f64 {
        self.f64
    }
    fn set_f64(&mut self, value: f64) {
        self.f64 = value;
        self.emit.f64_changed();
    }
    fn bytearray(&self) -> &[u8] {
        &self.bytearray
    }
    fn set_bytearray(&mut self, value: &[u8]) {
        self.bytearray.truncate(0);
        self.bytearray.extend_from_slice(value);
        self.emit.bytearray_changed();
    }
    fn optional_bytearray(&self) -> Option<&[u8]> {
        self.optional_bytearray.as_ref().map(|p|&p[..])
    }
    fn set_optional_bytearray(&mut self, value: Option<&[u8]>) {
        match (value, &mut self.optional_bytearray) {
            (Some(value), &mut Some(ref mut b)) => {
                b.truncate(0);
                b.extend_from_slice(value);
            },
            (Some(value), b) => {
                *b = Some(value.into())
            },
            (None, b) => {*b = None;}
        };
        self.emit.optional_bytearray_changed();
    }
    fn optional_string(&self) -> Option<&str> {
        self.optional_string.as_ref().map(|p|&p[..])
    }
    fn set_optional_string(&mut self, value: Option<String>) {
        self.optional_string = value;
        self.emit.optional_string_changed();
    }
    fn string(&self) -> &str {
        &self.string
    }
    fn set_string(&mut self, value: String) {
        self.string = value;
        self.emit.string_changed();
    }
}

