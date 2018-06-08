use interface::*;

#[derive(Default, Clone)]
struct TodosItem {
    completed: bool,
    description: String,
}

pub struct Todos {
    emit: TodosEmitter,
    model: TodosList,
    list: Vec<TodosItem>,
    active_count: usize,
}

impl Todos {
    fn update_active_count(&mut self) {
        let ac = self.list.iter().filter(|i| !i.completed).count();
        if self.active_count != ac {
            self.active_count = ac;
            self.emit.active_count_changed();
        }
    }
}

impl TodosTrait for Todos {
    fn new(emit: TodosEmitter, model: TodosList) -> Todos {
        Todos {
            emit: emit,
            model: model,
            list: vec![TodosItem::default(); 0],
            active_count: 0,
        }
    }
    fn emit(&self) -> &TodosEmitter {
        &self.emit
    }
    fn active_count(&self) -> u64 {
        self.active_count as u64
    }
    fn count(&self) -> u64 {
        self.list.len() as u64
    }
    fn row_count(&self) -> usize {
        self.list.len()
    }
    fn completed(&self, index: usize) -> bool {
        if index >= self.list.len() {
            return false;
        }
        self.list[index].completed
    }
    fn set_completed(&mut self, index: usize, v: bool) -> bool {
        if index >= self.list.len() {
            return false;
        }
        self.list[index].completed = v;
        self.update_active_count();
        true
    }
    fn description(&self, index: usize) -> &str {
        if index < self.list.len() {
            &self.list[index].description
        } else {
	    ""
        }
    }
    fn set_description(&mut self, index: usize, v: String) -> bool {
        if index >= self.list.len() {
            return false;
        }
        self.list[index].description = v;
        true
    }
    fn insert_rows(&mut self, row: usize, count: usize) -> bool {
        if count == 0 || row > self.list.len() {
            return false;
        }
        self.model.begin_insert_rows(row, row + count - 1);
        for i in 0..count {
            self.list.insert(row + i, TodosItem::default());
        }
        self.model.end_insert_rows();
        self.active_count += count;
        self.emit.active_count_changed();
        self.emit.count_changed();
        true
    }
    fn remove_rows(&mut self, row: usize, count: usize) -> bool {
        if count == 0 || row + count > self.list.len() {
            return false;
        }
        self.model.begin_remove_rows(row, row + count - 1);
        self.list.drain(row..row + count);
        self.model.end_remove_rows();
        self.emit.count_changed();
        self.update_active_count();
        true
    }
    fn clear_completed(&mut self) -> () {
        self.model.begin_reset_model();
        self.list.retain(|i| !i.completed);
        self.model.end_reset_model();
        self.emit.count_changed();
    }
    fn add(&mut self, description: String) {
        let end = self.list.len();
        self.model.begin_insert_rows(end, end);
        self.list.insert(end, TodosItem { completed: false, description });
        self.model.end_insert_rows();
        self.active_count += 1;
        self.emit.active_count_changed();
        self.emit.count_changed();
        self.model.begin_reset_model();
        self.model.end_reset_model();
    }
    fn remove(&mut self, index: u64) -> bool {
        self.remove_rows(index as usize, 1)
    }
    fn set_all(&mut self, completed: bool) {
        for i in &mut self.list {
            i.completed = completed;
        }
        self.model.data_changed(0, self.list.len() - 1);
        self.update_active_count();
    }
}
