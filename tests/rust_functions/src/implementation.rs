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
    
    fn double_name(&mut self) {
        self.user_name = format!("{}{}", self.user_name, self.user_name);
    }

    fn append(&mut self, suffix: String, amount: u32) {
        for _ in 0..amount {
            self.user_name += &suffix;
        }
    }

    fn greet(&self, name: String) -> String {
        format!("Hello {}, my name is {}, how is it going?", name, self.user_name)
    }

    fn vowels_in_name(&self) -> u8 {
        self.user_name.chars().fold(0, |count, ch| match ch {
            'a'|'e'|'i'|'o'|'u' => count + 1,
            _ => count
        })
    }

    fn quote(&self, prefix: String, suffix: String) -> String {
        format!("{}{}{}", prefix, self.user_name, suffix)
    }
    fn quote_bytes(&self, prefix: &[u8], suffix: &[u8]) -> Vec<u8> {
        let prefix = String::from_utf8_lossy(prefix);
        let suffix = String::from_utf8_lossy(suffix);
        format!("{}{}{}", prefix, self.user_name, suffix).into()
    }
}

