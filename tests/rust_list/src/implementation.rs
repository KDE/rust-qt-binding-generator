#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

#[derive(Default, Clone)]
struct PersonsItem {
    user_name: String,
    age: u8,
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

pub struct NoRole {
    emit: NoRoleEmitter,
    model: NoRoleList,
    list: Vec<PersonsItem>
}

impl NoRoleTrait for NoRole {
    fn new(emit: NoRoleEmitter, model: NoRoleList) -> NoRole {
        NoRole {
            emit: emit,
            model: model,
            list: vec![PersonsItem::default(); 10],
        }
    }
    fn emit(&self) -> &NoRoleEmitter {
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
    fn user_age(&self, item: usize) -> u8 {
        self.list[item].age
    }
    fn set_user_age(&mut self, item: usize, v: u8) -> bool {
        self.list[item].age = v;
        true
    }
}
