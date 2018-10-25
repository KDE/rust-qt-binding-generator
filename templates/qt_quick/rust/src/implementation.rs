use interface::*;

pub struct Simple {
    emit: SimpleEmitter,
    message: String,
}

impl SimpleTrait for Simple {
    fn new(emit: SimpleEmitter) -> Simple {
        Simple {
            emit: emit,
            message: String::new(),
        }
    }
    fn emit(&mut self) -> &mut SimpleEmitter {
        &mut self.emit
    }
    fn message(&self) -> &str {
        "Hello World!" 
    }
    fn set_message(&mut self, value: String) {
        self.message = value;
        self.emit.message_changed();
    }
}

