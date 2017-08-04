use interface::*;
use types::*;
use libc::{c_int};

pub struct Hello {
    emit: HelloEmitter,
    hello: String,
}

impl HelloTrait for Hello {
    fn create(emit: HelloEmitter) -> Self {
        Hello {
            emit: emit,
            hello: String::new()
        }
    }
    fn get_hello(&self) -> &String {
        &self.hello
    }
    fn set_hello(&mut self, value: String) {
        self.hello = value;
        self.emit.hello_changed();
    }
}

impl Drop for Hello {
    fn drop(&mut self) {
    }
}

pub struct RItemModel {
    emit: RItemModelEmitter,
//    string: String
}

impl RItemModelTrait for RItemModel {
    fn create(emit: RItemModelEmitter) -> Self {
        RItemModel {
            emit: emit
        }
    }
    fn column_count(&self, parent: QModelIndex) -> c_int {
        if parent.is_valid() { 0 } else { 2 }
    }
    fn row_count(&self, parent: QModelIndex) -> c_int {
        if parent.is_valid() { 0 } else { 2 }
    }
    fn index(&self, row: i32, column: i32, parent: QModelIndex) -> QModelIndex {
        QModelIndex::flat(row, column)
    }
    fn parent(&self, index: QModelIndex) -> QModelIndex {
        QModelIndex::invalid()
    }
    fn data<'a>(&'a self, index: QModelIndex, role: c_int) -> Variant<'a> {
        Variant::from("hello")
    }
}
