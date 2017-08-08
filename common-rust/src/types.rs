use std::slice;
use libc::{c_int, c_uint, uint8_t, uint16_t};
use std::ptr::null;
use std::marker::PhantomData;

#[repr(C)]
pub struct QString {
    data: *const uint8_t,
    len: c_int,
}

#[repr(C)]
pub struct QStringIn {
    data: *const uint16_t,
    len: c_int,
}

impl QStringIn {
    pub fn convert(&self) -> String {
        let data = unsafe { slice::from_raw_parts(self.data, self.len as usize) };
        String::from_utf16_lossy(data)
    }
}

impl<'a> From<&'a String> for QString {
    fn from(string: &'a String) -> QString {
        QString {
            len: string.len() as c_int,
            data: string.as_ptr(),
        }
    }
}

#[repr(C)]
pub struct QByteArray {
    data: *const uint8_t,
    len: c_int,
}

impl QByteArray {
    pub fn convert(&self) -> Vec<u8> {
        let data = unsafe { slice::from_raw_parts(self.data, self.len as usize) };
        Vec::from(data)
    }
}

impl<'a> From<&'a Vec<u8>> for QByteArray {
    fn from(value: &'a Vec<u8>) -> QByteArray {
        QByteArray {
            len: value.len() as c_int,
            data: value.as_ptr(),
        }
    }
}

pub enum Variant {
    None,
    Bool(bool),
    String(String),
    ByteArray(Vec<u8>),
}

impl From<bool> for Variant {
    fn from(value: bool) -> Variant {
        Variant::Bool(value)
    }
}

impl From<String> for Variant {
    fn from(value: String) -> Variant {
        Variant::String(value)
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
    phantom: PhantomData<&'a u8>,
}

impl<'a> QVariant<'a> {
    pub fn type_(&self) -> u32 {
        self.type_ as u32
    }
    pub fn convert(&self) -> Variant {
        // TODO
        Variant::None
    }
}

impl<'a> From<&'a Variant> for QVariant<'a> {
    fn from(variant: &'a Variant) -> QVariant {
        match *variant {
            Variant::None => {
                QVariant {
                    data: null(),
                    len: 0,
                    type_: VariantType::Invalid as c_uint,
                    phantom: PhantomData,
                }
            }
            Variant::Bool(v) => {
                QVariant {
                    data: null(),
                    len: v as c_int,
                    type_: VariantType::Bool as c_uint,
                    phantom: PhantomData,
                }
            }
            Variant::String(ref v) => {
                QVariant {
                    data: v.as_ptr(),
                    len: v.len() as c_int,
                    type_: VariantType::String as c_uint,
                    phantom: PhantomData,
                }
            }
            Variant::ByteArray(ref v) => {
                QVariant {
                    data: v.as_ptr(),
                    len: v.len() as c_int,
                    type_: VariantType::ByteArray as c_uint,
                    phantom: PhantomData,
                }
            }
        }
    }
}

#[repr(C)]
pub struct QModelIndex {
    row: c_int,
    column: c_int,
    internal_id: usize,
}

impl QModelIndex {
    pub fn invalid() -> QModelIndex {
        QModelIndex {
            row: -1,
            column: -1,
            internal_id: 0,
        }
    }
    pub fn create(row: c_int, column: c_int, id: usize) -> QModelIndex {
        QModelIndex {
            row: row,
            column: column,
            internal_id: id,
        }
    }
    pub fn flat(row: c_int, column: c_int) -> QModelIndex {
        QModelIndex {
            row: row,
            column: column,
            internal_id: 1,
        }
    }
    pub fn is_valid(&self) -> bool {
        self.internal_id != 0 && self.row >= 0 && self.column >= 0
    }
    pub fn row(&self) -> c_int {
        self.row
    }
    pub fn column(&self) -> c_int {
        self.column
    }
    pub fn id(&self) -> usize {
        self.internal_id
    }
}
