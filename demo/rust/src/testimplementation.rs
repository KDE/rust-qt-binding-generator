#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use libc::c_int;
use libc::c_uint;
use types::*;
use testinterface::*;

pub struct Person {
    emit: PersonEmitter,
    user_name: String,
    age: c_int,
    active: bool,
    icon: Vec<u8>,
}

impl PersonTrait for Person {
    fn create(emit: PersonEmitter) -> Person {
        Person {
            emit: emit,
            user_name: String::new(),
            age: 0,
            active: true,
            icon: Vec::new(),
        }
    }
    fn emit(&self) -> &PersonEmitter {
        &self.emit
    }
    fn get_user_name(&self) -> String {
        self.user_name.clone()
    }
    fn set_user_name(&mut self, value: String) {
        self.user_name = value;
        self.emit.user_name_changed();
    }
    fn get_age(&self) -> c_int {
        self.age
    }
    fn get_active(&self) -> bool {
        self.active
    }
    fn set_active(&mut self, value: bool) {
        self.active = value;
        self.emit.active_changed();
    }
    fn get_icon(&self) -> Vec<u8> {
        self.icon.clone()
    }
    fn set_icon(&mut self, value: Vec<u8>) {
        self.icon = value;
        self.emit.icon_changed();
    }
}
pub struct Directory {
    emit: DirectoryEmitter,
    model: DirectoryList,
    path: String,
}

impl DirectoryTrait for Directory {
    fn create(emit: DirectoryEmitter, model: DirectoryList) -> Directory {
        Directory {
            emit: emit,
            model: model,
            path: String::new(),
        }
    }
    fn emit(&self) -> &DirectoryEmitter {
        &self.emit
    }
    fn get_path(&self) -> String {
        self.path.clone()
    }
    fn set_path(&mut self, value: String) {
        self.path = value;
        self.emit.path_changed();
    }
    fn row_count(&self) -> c_int {
        10
    }
    fn file_name(&self, row: c_int) -> String {
        String::new()
    }
    fn file_icon(&self, row: c_int) -> Vec<u8> {
        Vec::new()
    }
    fn file_path(&self, row: c_int) -> String {
        String::new()
    }
    fn file_permissions(&self, row: c_int) -> c_int {
        0
    }
}
pub struct TestTree {
    emit: TestTreeEmitter,
    model: TestTreeUniformTree,
    path: String,
}

impl TestTreeTrait for TestTree {
    fn create(emit: TestTreeEmitter, model: TestTreeUniformTree) -> TestTree {
        TestTree {
            emit: emit,
            model: model,
            path: String::new(),
        }
    }
    fn emit(&self) -> &TestTreeEmitter {
        &self.emit
    }
    fn get_path(&self) -> String {
        self.path.clone()
    }
    fn set_path(&mut self, value: String) {
        self.path = value;
        self.emit.path_changed();
    }
    fn row_count(&self, row: c_int, parent: usize) -> c_int {
        10
    }
    fn file_name(&self, row: c_int, parent: usize) -> String {
        String::new()
    }
    fn file_icon(&self, row: c_int, parent: usize) -> Vec<u8> {
        Vec::new()
    }
    fn file_path(&self, row: c_int, parent: usize) -> String {
        String::new()
    }
    fn file_permissions(&self, row: c_int, parent: usize) -> c_int {
        0
    }
    fn index(&self, row: c_int, parent: usize) -> usize {
        0
    }
    fn parent(&self, parent: usize) -> QModelIndex {
        QModelIndex::create(0, 0)
    }
}
