#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

pub struct Group {
    emit: GroupEmitter,
    person: Person,
}

impl GroupTrait for Group {
    fn new(emit: GroupEmitter, person: Person) -> Group {
        Group {
            emit: emit,
            person: person,
        }
    }
    fn emit(&self) -> &GroupEmitter {
        &self.emit
    }
    fn person(&self) -> &Person {
        &self.person
    }
    fn person_mut(&mut self) -> &mut Person {
        &mut self.person
    }
}

pub struct InnerObject {
    emit: InnerObjectEmitter,
    description: String,
}

impl InnerObjectTrait for InnerObject {
    fn new(emit: InnerObjectEmitter) -> InnerObject {
        InnerObject {
            emit: emit,
            description: String::new(),
        }
    }
    fn emit(&self) -> &InnerObjectEmitter {
        &self.emit
    }
    fn description(&self) -> &str {
        &self.description
    }
    fn set_description(&mut self, value: String) {
        self.description = value;
        self.emit.description_changed();
    }
}

pub struct Person {
    emit: PersonEmitter,
    object: InnerObject,
}

impl PersonTrait for Person {
    fn new(emit: PersonEmitter, object: InnerObject) -> Person {
        Person {
            emit: emit,
            object: object,
        }
    }
    fn emit(&self) -> &PersonEmitter {
        &self.emit
    }
    fn object(&self) -> &InnerObject {
        &self.object
    }
    fn object_mut(&mut self) -> &mut InnerObject {
        &mut self.object
    }
}

