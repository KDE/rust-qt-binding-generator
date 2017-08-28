#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use interface::*;

pub struct Group {
    emit: GroupEmitter,
    person: Person,
}

impl GroupTrait for Group {
    fn create(emit: GroupEmitter, person: Person) -> Group {
        Group {
            emit: emit,
            person: person,
        }
    }
    fn emit(&self) -> &GroupEmitter {
        &self.emit
    }
    fn get_person(&self) -> &Person {
        &self.person
    }
    fn get_mut_person(&mut self) -> &mut Person {
        &mut self.person
    }
}

pub struct InnerObject {
    emit: InnerObjectEmitter,
    description: String,
}

impl InnerObjectTrait for InnerObject {
    fn create(emit: InnerObjectEmitter) -> InnerObject {
        InnerObject {
            emit: emit,
            description: String::new(),
        }
    }
    fn emit(&self) -> &InnerObjectEmitter {
        &self.emit
    }
    fn get_description(&self) -> String {
        self.description.clone()
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
    fn create(emit: PersonEmitter, object: InnerObject) -> Person {
        Person {
            emit: emit,
            object: object,
        }
    }
    fn emit(&self) -> &PersonEmitter {
        &self.emit
    }
    fn get_object(&self) -> &InnerObject {
        &self.object
    }
    fn get_mut_object(&mut self) -> &mut InnerObject {
        &mut self.object
    }
}

