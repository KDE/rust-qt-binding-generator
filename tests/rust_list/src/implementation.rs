#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

#[derive(Default, Clone)]
struct PersonsItem {
    user_name: String,
}

pub struct Persons {
    emit: PersonsEmitter,
    model: PersonsList,
    list: Vec<PersonsItem>,
}

impl PersonsTrait for Persons {
    fn new(emit: PersonsEmitter, model: PersonsList) -> Persons {
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
    fn user_name(&self, item: usize) -> &str {
        &self.list[item].user_name
    }
    fn set_user_name(&mut self, item: usize, v: String) -> bool {
        self.list[item].user_name = v;
        true
    }
}

