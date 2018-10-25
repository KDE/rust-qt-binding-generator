#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

pub struct Person {
    emit: PersonEmitter,
    user_name: String,
}

impl PersonTrait for Person {
    fn new(emit: PersonEmitter) -> Person {
        Person {
            emit: emit,
            user_name: String::new(),
        }
    }
    fn emit(&mut self) -> &mut PersonEmitter {
        &mut self.emit
    }
    fn user_name(&self) -> &str {
        &self.user_name
    }
    fn set_user_name(&mut self, value: String) {
        self.user_name = value;
        self.emit.user_name_changed();
    }
}

