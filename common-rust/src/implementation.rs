use interface::*;
use types::*;
use libc::{c_int};
use std::fs::read_dir;
use std::path::PathBuf;
use std::ffi::OsString;
use std::default::Default;

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

pub struct DirEntry {
    name: OsString,
}

impl DirEntry {
    pub fn create(name: &str) -> DirEntry {
        DirEntry {
            name: OsString::from(name)
        }
    }
}

impl Item for DirEntry {
    fn data<'a>(&self, role: c_int) -> Variant<'a> {
        let str = self.name.to_string_lossy().to_string();
        return Variant::from(str);
    }
    fn retrieve(&self, parents: Vec<&DirEntry>) -> Vec<DirEntry> {
        let path: PathBuf = parents.into_iter().map(|e|&e.name).collect();
        let mut v = Vec::new();
        if let Ok(it) = read_dir(path) {
            for i in it.filter_map(|v|v.ok()) {
                let de = DirEntry {
                    name: i.file_name(),
                };
                v.push(de);
            }
        }
        v.sort_by_key(|ref i| i.name.clone());
        return v;
    }
}

impl Default for DirEntry {
    fn default() -> DirEntry {
        DirEntry {
            name: OsString::new()
        }
    }
}

pub trait Item: Default {
    fn retrieve(&self, parents: Vec<&Self>) -> Vec<Self>;
    fn data<'a>(&self, role: c_int) -> Variant<'a>;
}

pub type RItemModel = RGeneralItemModel<DirEntry>;

struct Entry<T: Item> {
    parent: usize,
    row: usize,
    children: Option<Vec<usize>>,
    data: T
}

pub struct RGeneralItemModel<T: Item> {
    emit: RItemModelEmitter,
    entries: Vec<Entry<T>>
}

impl<T: Item> RGeneralItemModel<T> {
    fn get(&mut self, index: &QModelIndex) -> usize {
        let p = if index.is_valid() {
            let row = index.row() as usize;
            self.entries[index.id() as usize].children.as_ref().unwrap()[row]
        } else {
            1
        };
        if self.entries[p].children.is_none() {
            self.retrieve(p);
        }
        p
    }
    fn retrieve(&mut self, id: usize) {
        let mut new_entries = Vec::new();
        let mut children = Vec::new();
        {
            let mut row = 0;
            let parents = self.get_parents(id);
            let entries = self.entries[id].data.retrieve(parents);
            for d in entries {
                let e = Entry {
                    parent: id,
                    row: row,
                    children: None,
                    data: d
                };
                children.push(self.entries.len() + row);
                row += 1;
                new_entries.push(e);
            }
        }
        self.entries[id].children = Some(children);
        self.entries.append(&mut new_entries);
    }
    fn get_parents(&self, id: usize) -> Vec<&T> {
        let mut pos = id;
        let mut e = Vec::new();
        while pos > 0 {
            e.push(pos);
            pos = self.entries[pos].parent;
        }
        e.into_iter().rev().map(|i| &self.entries[i].data).collect()
    }
}

impl<T: Item> RItemModelTrait<T> for RGeneralItemModel<T> {
    fn create(emit: RItemModelEmitter, root: T) -> Self {
        let none = Entry { parent: 0, row: 0, children: None, data: T::default() };
        let root = Entry { parent: 0, row: 0, children: None, data: root };
        RGeneralItemModel {
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
        let ref e = self.entries[index.id()];
        QModelIndex::create(e.row as i32, 0, e.parent)
    }
    fn data<'a>(&'a mut self, index: QModelIndex, role: c_int) -> Variant<'a> {
        let i = self.get(&index);
        return self.entries[i].data.data(role);
    }
}
