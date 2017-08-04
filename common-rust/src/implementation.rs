use interface::*;
use types::*;
use libc::{c_int};
use std::fs::read_dir;
use std::path::PathBuf;
use std::ffi::OsString;

pub struct Hello {
    emit: HelloEmitter,
    hello: String,
}

impl HelloTrait for Hello {
    fn create(emit: HelloEmitter) -> Self {
        Hello {
            emit: emit,
            hello: String::new()
        }
    }
    fn get_hello(&self) -> &String {
        &self.hello
    }
    fn set_hello(&mut self, value: String) {
        self.hello = value;
        self.emit.hello_changed();
    }
}

impl Drop for Hello {
    fn drop(&mut self) {
    }
}

struct DirEntry {
    name: OsString,
    parent: usize,
    children: Option<Vec<usize>>
}

pub struct RItemModel {
    emit: RItemModelEmitter,
    entries: Vec<DirEntry>
}

impl RItemModel {
    fn get(&mut self, index: &QModelIndex) -> usize {
        let p = if index.is_valid() {
            let row = index.row() as usize;
            self.entries[index.id() as usize].children.as_ref().unwrap()[row]
        } else {
            1
        };
        if self.entries[p].children.is_none() {
            self.read_dir(p);
        }
        p
    }
    fn get_path(&self, id: usize) -> PathBuf {
        let mut pos = id;
        let mut e = Vec::new();
        while pos > 0 {
            e.push(pos);
            pos = self.entries[pos].parent;
        }
        let mut path = PathBuf::new();
        for i in e.into_iter().rev() {
            path.push(&self.entries[i].name);
        }
        path
    }
    fn read_dir(&mut self, id: usize) {
        let mut v = Vec::new();
        if let Ok(it) = read_dir(self.get_path(id)) {
            for i in it.filter_map(|v|v.ok()) {
                let de = DirEntry {
                    name: i.file_name(),
                    parent: id,
                    children: None
                };
                v.push(self.entries.len());
                self.entries.push(de);
            }
        }
        self.entries[id].children = Some(v);
    }
}

impl RItemModelTrait for RItemModel {
    fn create(emit: RItemModelEmitter) -> Self {
        let none = DirEntry { name: OsString::new(), parent: 0, children: None };
        let root = DirEntry { name: OsString::from("/"), parent: 0, children: None };
        RItemModel {
            emit: emit,
            entries: vec![none, root]
        }
    }
    fn column_count(&mut self, parent: QModelIndex) -> c_int {
        1
    }
    fn row_count(&mut self, parent: QModelIndex) -> c_int {
        let i = self.get(&parent);
        self.entries[i].children.as_ref().unwrap().len() as i32
    }
    fn index(&mut self, row: i32, column: i32, parent: QModelIndex) -> QModelIndex {
        QModelIndex::create(row, column, self.get(&parent))
    }
    fn parent(&self, index: QModelIndex) -> QModelIndex {
        if !index.is_valid() || index.id() == 1 {
            return QModelIndex::invalid();
        }
        let parentid = self.entries[index.id()].parent;
        let ref e = self.entries[parentid];
        let mut i = e.children.as_ref().unwrap().iter();
        let row = i.position(|v|*v==index.id()).unwrap() as i32;
        QModelIndex::create(row, 0, e.parent)
    }
    fn data<'a>(&'a mut self, index: QModelIndex, role: c_int) -> Variant<'a> {
        let i = self.get(&index);
        return Variant::from(self.entries[i].name.to_string_lossy().to_string());
    }
}
