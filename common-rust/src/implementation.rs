use interface::*;

pub struct Hello {
    notifier: HelloNotifier,
    hello: String,
}

impl HelloTrait for Hello {
    fn create(notifier: HelloNotifier) -> Self {
        Hello {
            notifier: notifier,
            hello: String::new()
        }
    }
    fn get_hello(&self) -> &String {
        &self.hello
    }
    fn set_hello(&mut self, value: String) {
        self.hello = value;
        self.notifier.hello_changed();
    }
}

impl Drop for Hello {
    fn drop(&mut self) {
    }
}

pub struct RItemModel {
    notifier: RItemModelNotifier
}

impl RItemModelTrait for RItemModel {
    fn create(notifier: RItemModelNotifier) -> Self {
        RItemModel {
            notifier: notifier
        }
    }
}
