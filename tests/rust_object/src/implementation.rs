#![allow(unused_imports)]
use libc::c_int;
use types::*;
use interface::*;

pub struct Person {
    emit: PersonEmitter,
    user_name: String,
}

impl PersonTrait for Person {
    fn create(emit: PersonEmitter) -> Person {
        Person {
            emit: emit,
            user_name: String::new(),
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
}
