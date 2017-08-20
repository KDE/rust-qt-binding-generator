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
    model: PersonsList,
    list: Vec<PersonsItem>,
}

impl PersonsTrait for Persons {
    fn create(emit: PersonsEmitter, model: PersonsList) -> Persons {
        Persons {
            emit: emit,
            model: model,
            list: vec![PersonsItem::default(); 10],
        }
    }
    fn emit(&self) -> &PersonsEmitter {
        &self.emit
    }
    fn row_count(&self) -> c_int {
        self.list.len() as c_int
    }
    fn user_name(&self, row: c_int) -> String {
        self.list[row as usize].user_name.clone()
    }
    fn set_user_name(&mut self, row: c_int, v: String) -> bool {
        self.list[row as usize].user_name = v;
        true
    }
}
