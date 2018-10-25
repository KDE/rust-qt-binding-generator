#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

#[derive(Default, Clone)]
struct ListItem {
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
    f32: f32,
    f64: f64,
    bytearray: Vec<u8>,
    optional_bytearray: Option<Vec<u8>>,
    string: String,
    optional_string: Option<String>,
}

pub struct List {
    emit: ListEmitter,
    model: ListList,
    list: Vec<ListItem>,
}

impl ListTrait for List {
    fn new(emit: ListEmitter, model: ListList) -> List {
        List {
            emit: emit,
            model: model,
            list: vec![ListItem::default(); 10],
        }
    }
    fn emit(&mut self) -> &mut ListEmitter {
        &mut self.emit
    }
    fn row_count(&self) -> usize {
        self.list.len()
    }
    fn boolean(&self, index: usize) -> bool {
        self.list[index].boolean
    }
    fn set_boolean(&mut self, index: usize, v: bool) -> bool {
        self.list[index].boolean = v;
        true
    }
    fn optional_boolean(&self, index: usize) -> Option<bool> {
        self.list[index].optional_boolean
    }
    fn set_optional_boolean(&mut self, index: usize, v: Option<bool>) -> bool {
        self.list[index].optional_boolean = v;
        true
    }
    fn i8(&self, index: usize) -> i8 {
        self.list[index].i8
    }
    fn set_i8(&mut self, index: usize, v: i8) -> bool {
        self.list[index].i8 = v;
        true
    }
    fn u8(&self, index: usize) -> u8 {
        self.list[index].u8
    }
    fn set_u8(&mut self, index: usize, v: u8) -> bool {
        self.list[index].u8 = v;
        true
    }
    fn i16(&self, index: usize) -> i16 {
        self.list[index].i16
    }
    fn set_i16(&mut self, index: usize, v: i16) -> bool {
        self.list[index].i16 = v;
        true
    }
    fn u16(&self, index: usize) -> u16 {
        self.list[index].u16
    }
    fn set_u16(&mut self, index: usize, v: u16) -> bool {
        self.list[index].u16 = v;
        true
    }
    fn i32(&self, index: usize) -> i32 {
        self.list[index].i32
    }
    fn set_i32(&mut self, index: usize, v: i32) -> bool {
        self.list[index].i32 = v;
        true
    }
    fn u32(&self, index: usize) -> u32 {
        self.list[index].u32
    }
    fn set_u32(&mut self, index: usize, v: u32) -> bool {
        self.list[index].u32 = v;
        true
    }
    fn i64(&self, index: usize) -> i64 {
        self.list[index].i64
    }
    fn set_i64(&mut self, index: usize, v: i64) -> bool {
        self.list[index].i64 = v;
        true
    }
    fn u64(&self, index: usize) -> u64 {
        self.list[index].u64
    }
    fn set_u64(&mut self, index: usize, v: u64) -> bool {
        self.list[index].u64 = v;
        true
    }
    fn f32(&self, index: usize) -> f32 {
        self.list[index].f32
    }
    fn set_f32(&mut self, index: usize, v: f32) -> bool {
        self.list[index].f32 = v;
        true
    }
    fn f64(&self, index: usize) -> f64 {
        self.list[index].f64
    }
    fn set_f64(&mut self, index: usize, v: f64) -> bool {
        self.list[index].f64 = v;
        true
    }
    fn string(&self, index: usize) -> &str {
        &self.list[index].string
    }
    fn set_string(&mut self, index: usize, v: String) -> bool {
        self.list[index].string = v;
        true
    }
    fn optional_string(&self, index: usize) -> Option<&str> {
        self.list[index].optional_string.as_ref().map(|p|&p[..])
    }
    fn set_optional_string(&mut self, index: usize, v: Option<String>) -> bool {
        self.list[index].optional_string = v;
        true
    }
    fn bytearray(&self, index: usize) -> &[u8] {
        &self.list[index].bytearray
    }
    fn set_bytearray(&mut self, index: usize, v: &[u8]) -> bool {
        self.list[index].bytearray.truncate(0);
        self.list[index].bytearray.extend_from_slice(v);
        true
    }
    fn optional_bytearray(&self, index: usize) -> Option<&[u8]> {
        self.list[index].optional_bytearray.as_ref().map(|p|&p[..])
    }
    fn set_optional_bytearray(&mut self, index: usize, v: Option<&[u8]>) -> bool {
        match (v, &mut self.list[index].optional_bytearray) {
            (Some(value), &mut Some(ref mut b)) => {
                b.truncate(0);
                b.extend_from_slice(value);
            },
            (Some(value), b) => {
                *b = Some(value.into())
            },
            (None, b) => {*b = None;}
        };
        true
    }
}

