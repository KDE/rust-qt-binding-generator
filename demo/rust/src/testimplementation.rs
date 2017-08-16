#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use libc::c_int;
use libc::c_uint;
use types::*;
use testinterface::*;

pub struct Fibonacci {
    emit: FibonacciEmitter,
    input: c_uint,
    result: u64,
}

impl FibonacciTrait for Fibonacci {
    fn create(emit: FibonacciEmitter) -> Fibonacci {
        Fibonacci {
            emit: emit,
            input: 0,
            result: 0,
        }
    }
    fn emit(&self) -> &FibonacciEmitter {
        &self.emit
    }
    fn get_input(&self) -> c_uint {
        self.input
    }
    fn set_input(&mut self, value: c_uint) {
        self.input = value;
        self.emit.input_changed();
    }
    fn get_result(&self) -> u64 {
        self.result
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
