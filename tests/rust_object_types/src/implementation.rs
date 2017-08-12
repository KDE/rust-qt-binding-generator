use libc::c_int;
use types::*;
use interface::*;

pub struct Object {
    emit: ObjectEmitter,
    value: Variant,
}

impl ObjectTrait for Object {
    fn create(emit: ObjectEmitter) -> Object {
        Object {
            emit: emit,
            value: Variant::None,
        }
    }
    fn emit(&self) -> &ObjectEmitter {
        &self.emit
    }
    fn get_value(&self) -> Variant {
        self.value.clone()
    }
    fn set_value(&mut self, value: Variant) {
        self.value = value;
        self.emit.value_changed();
    }
}
