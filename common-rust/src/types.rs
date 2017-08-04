use libc::{c_int, c_uint, uint8_t};
use std::ptr::null;
use std::marker::PhantomData;

#[repr(C)]
pub struct QString {
    data: *const uint8_t,
    len: c_int
}

impl<'a> From<&'a String> for QString {
    fn from(string: &'a String) -> QString {
        QString {
            len: string.len() as c_int,
            data: string.as_ptr()
        }
    }
}

pub enum Variant<'a> {
    None,
    Bool(bool),
    String(String),
    str(&'a str)
}

impl<'a> From<bool> for Variant<'a> {
    fn from(value: bool) -> Variant<'a> {
        Variant::Bool(value)
    }
}

impl<'a> From<String> for Variant<'a> {
    fn from(value: String) -> Variant<'a> {
        Variant::String(value)
    }
}

impl<'a> From<&'a str> for Variant<'a> {
    fn from(value: &'a str) -> Variant {
        Variant::str(value)
    }
}

/* values from qvariant.h and qmetatype.h */
#[repr(u32)]
#[derive(Clone, Copy)]
enum VariantType {
    Invalid = 0,
    Bool = 1,
    String = 10,
    ByteArray = 12,
}

#[repr(C)]
pub struct QVariant<'a> {
    type_: c_uint,
    len: c_int,
    data: *const uint8_t,
    phantom: PhantomData<&'a u8>
}

impl<'a> QVariant<'a> {
    pub fn type_(&self) -> u32 {
        self.type_ as u32
    }
}

impl<'a> From<&'a Variant<'a>> for QVariant<'a> {
    fn from(variant: &'a Variant<'a>) -> QVariant {
        match variant {
            &Variant::None => QVariant {
                data: null(),
                len: 0,
                type_: VariantType::Invalid as c_uint,
                phantom: PhantomData
            },
            &Variant::Bool(v) => QVariant {
                data: null(),
                len: v as c_int,
                type_: VariantType::Bool as c_uint,
                phantom: PhantomData
            },
            &Variant::String(ref v) => QVariant {
                data: v.as_ptr(),
                len: v.len() as c_int,
                type_: VariantType::String as c_uint,
                phantom: PhantomData
            },
            &Variant::str(ref v) => QVariant {
                data: v.as_ptr(),
                len: v.len() as c_int,
                type_: VariantType::String as c_uint,
                phantom: PhantomData
            }
        }
    }
}

#[repr(C)]
pub struct QModelIndex {
    row: c_int,
    column: c_int,
    internal_id: usize
}

impl QModelIndex {
    pub fn invalid() -> QModelIndex {
        QModelIndex {
            row: -1,
            column: -1,
            internal_id: 0
        }
    }
    pub fn flat(row: c_int, column: c_int) -> QModelIndex {
        QModelIndex {
            row: row,
            column: column,
            internal_id: 1
        }
    }
    pub fn is_valid(&self) -> bool {
        self.internal_id != 0 && self.row >= 0 && self.column >= 0
    }
}
