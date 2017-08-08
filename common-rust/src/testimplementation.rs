use libc::{c_int};
use types::*;
use testinterface::*;

pub struct Test {
    emit: TestEmitter,
    username: String,
    age: c_int,
    active: bool,
    misc: Variant
}

impl TestTrait for Test {
    fn create(emit: TestEmitter) -> Test {
        Test {
            emit: emit,
            username: String::new(),
            age: 0,
            active: true,
            misc: Variant::None
        }
    }
    fn emit(&self) -> &TestEmitter {
        &self.emit
    }
    fn get_username(&self) -> String { self.username.clone() }
    fn set_username(&mut self, value: String) {
        self.username = value;
        self.emit.username_changed();
    }
    fn get_age(&self) -> c_int {
        self.age
    }
    fn get_active(&self) -> bool {
        self.active
    }
    fn set_active(&mut self, value: bool) {
        self.active = value;
        self.emit.active_changed();
    }
    fn get_misc(&self) -> Variant { Variant::None }
    fn set_misc(&mut self, value: Variant) {
        self.misc = value;
        self.emit.misc_changed();
    }
}
