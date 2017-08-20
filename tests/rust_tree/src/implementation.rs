#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use libc::c_int;
use libc::c_uint;
use types::*;
use interface::*;

#[derive (Default, Clone)]
struct PersonsItem {
    user_name: String,
}

pub struct Persons {
    emit: PersonsEmitter,
    model: PersonsUniformTree,
    list: Vec<PersonsItem>,
}

impl PersonsTrait for Persons {
    fn create(emit: PersonsEmitter, model: PersonsUniformTree) -> Persons {
        Persons {
            emit: emit,
            model: model,
            list: vec![PersonsItem::default(); 10],
        }
    }
    fn emit(&self) -> &PersonsEmitter {
        &self.emit
    }
    fn row_count(&self, row: c_int, parent: usize) -> c_int {
        self.list.len() as c_int
    }
    fn user_name(&self, row: c_int, parent: usize) -> String {
        self.list[row as usize].user_name.clone()
    }
    fn set_user_name(&mut self, row: c_int, parent: usize, v: String) -> bool {
        self.list[row as usize].user_name = v;
        true
    }
    fn index(&self, row: c_int, parent: usize) -> usize {
        0
    }
    fn parent(&self, parent: usize) -> QModelIndex {
        QModelIndex::create(0, 0)
    }
}
