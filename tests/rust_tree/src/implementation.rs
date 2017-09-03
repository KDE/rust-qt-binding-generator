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
    model: PersonsTree,
    list: Vec<PersonsItem>,
}

impl PersonsTrait for Persons {
    fn new(emit: PersonsEmitter, model: PersonsTree) -> Persons {
        Persons {
            emit: emit,
            model: model,
            list: vec![PersonsItem::default(); 10],
        }
    }
    fn emit(&self) -> &PersonsEmitter {
        &self.emit
    }
    fn row_count(&self, item: Option<usize>) -> usize {
        self.list.len()
    }
    fn index(&self, item: Option<usize>, row: usize) -> usize {
        0
    }
    fn parent(&self, item: usize) -> Option<usize> {
        None
    }
    fn row(&self, item: usize) -> usize {
        item
    }
    fn user_name(&self, item: usize) -> &str {
        &self.list[item].user_name
    }
    fn set_user_name(&mut self, item: usize, v: String) -> bool {
        self.list[item].user_name = v;
        true
    }
}

