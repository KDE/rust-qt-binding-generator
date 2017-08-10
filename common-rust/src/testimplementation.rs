use libc::c_int;
use types::*;
use testinterface::*;

pub struct Person {
    emit: PersonEmitter,
    user_name: String,
    age: c_int,
    active: bool,
    misc: Variant,
    icon: Vec<u8>,
}

impl PersonTrait for Person {
    fn create(emit: PersonEmitter) -> Person {
        Person {
            emit: emit,
            user_name: String::new(),
            age: 0,
            active: true,
            misc: Variant::None,
            icon: Vec::new(),
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
    fn set_path(&mut self, value: String) {
        self.path = value;
        self.emit.path_changed();
    }
    fn row_count(&self) -> c_int {
        10
    }
    fn file_icon(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
    fn file_path(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
    fn file_name(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
    fn file_permissions(&self, row: c_int) -> Variant {
        Variant::Bool(row > 0)
    }
}
