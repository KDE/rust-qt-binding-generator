#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
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
    fn row_count(&self) -> usize {
        self.list.len()
    }
    fn user_name(&self, item: usize) -> String {
        self.list[item].user_name.clone()
    }
    fn set_user_name(&mut self, item: usize, v: String) -> bool {
        self.list[item].user_name = v;
        true
    }
}
