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
    fn row_count(&self, index: Option<usize>) -> usize {
        self.list.len()
    }
    fn index(&self, index: Option<usize>, row: usize) -> usize {
        0
    }
    fn parent(&self, index: usize) -> Option<usize> {
        None
    }
    fn row(&self, index: usize) -> usize {
        index
    }
    fn user_name(&self, index: usize) -> &str {
        &self.list[index].user_name
    }
    fn set_user_name(&mut self, index: usize, v: String) -> bool {
        self.list[index].user_name = v;
        true
    }
}

