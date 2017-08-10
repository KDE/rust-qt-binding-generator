use libc::c_int;
use types::*;
use testinterface::*;

pub struct Test {
    emit: TestEmitter,
    username: String,
    age: c_int,
    active: bool,
    misc: Variant,
    icon: Vec<u8>,
}

impl TestTrait for Test {
    fn create(emit: TestEmitter) -> Test {
        Test {
            emit: emit,
            username: String::new(),
            age: 0,
            active: true,
            misc: Variant::None,
            icon: Vec::new(),
        }
    }
    fn emit(&self) -> &TestEmitter {
        &self.emit
    }
    fn get_username(&self) -> String {
        self.username.clone()
    }
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
    fn get_misc(&self) -> Variant {
        self.misc.clone()
    }
    fn set_misc(&mut self, value: Variant) {
        self.misc = value;
        self.emit.misc_changed();
    }
    fn get_icon(&self) -> Vec<u8> {
        self.icon.clone()
    }
    fn set_icon(&mut self, value: Vec<u8>) {
        self.icon = value;
        self.emit.icon_changed();
    }
}
pub struct Directory {
    emit: DirectoryEmitter,
    model: DirectoryList,
    path: String,
}

impl DirectoryTrait for Directory {
    fn create(emit: DirectoryEmitter, model: DirectoryList) -> Directory {
        Directory {
            emit: emit,
            model: model,
            path: String::new(),
        }
    }
    fn emit(&self) -> &DirectoryEmitter {
        &self.emit
    }
    fn get_path(&self) -> String {
        self.path.clone()
    }
    fn row_count(&self) -> c_int {
        0
    }
    fn fileicon(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
    fn filepath(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
    fn filename(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
    fn filepermissions(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
}
