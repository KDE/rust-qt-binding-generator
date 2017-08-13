use interface::*;
use types::*;
use libc::c_int;
use std::fs::read_dir;
use std::path::PathBuf;
use std::ffi::OsString;
use std::default::Default;
use std::thread;

pub struct DirEntry {
    name: OsString,
}

impl DirEntry {
    pub fn create(name: &str) -> DirEntry {
        DirEntry { name: OsString::from(name) }
    }
}

impl Item for DirEntry {
    fn file_name(&self) -> String {
        self.name.to_string_lossy().to_string()
    }
    fn file_permissions(&self) -> c_int {
        42
    }
    fn retrieve(&self, parents: Vec<&DirEntry>) -> Vec<DirEntry> {
        let path: PathBuf = parents.into_iter().map(|e| &e.name).collect();
        let mut v = Vec::new();
        if let Ok(it) = read_dir(path) {
            for i in it.filter_map(|v| v.ok()) {
                let de = DirEntry { name: i.file_name() };
                v.push(de);
            }
        }
        v.sort_by(|a, b| a.name.cmp(&b.name));
        v
    }
}

impl Default for DirEntry {
    fn default() -> DirEntry {
        DirEntry { name: OsString::new() }
    }
}

pub trait Item: Default {
    fn retrieve(&self, parents: Vec<&Self>) -> Vec<Self>;
    fn file_name(&self) -> String;
    fn file_permissions(&self) -> c_int;
}

pub type Tree = RGeneralItemModel<DirEntry>;

struct Entry<T: Item> {
    parent: usize,
    row: usize,
    children: Option<Vec<usize>>,
    data: T,
}

pub struct RGeneralItemModel<T: Item> {
    emit: TreeEmitter,
    entries: Vec<Entry<T>>,
}

impl<T: Item> RGeneralItemModel<T> {
    fn get(&mut self, row: c_int, parent: usize) -> usize {
        let p = if parent > 0 {
            self.entries[parent].children.as_ref().unwrap()[row as usize]
        } else {
            1
        };
        if self.entries[p].children.is_none() {
            self.retrieve(p);
            let emit = self.emit.clone();
//            thread::spawn(move || { emit.new_data_ready(); });
        }
        p
    }
    fn retrieve(&mut self, id: usize) {
        let mut new_entries = Vec::new();
        let mut children = Vec::new();
        {
            let parents = self.get_parents(id);
            let entries = self.entries[id].data.retrieve(parents);
            for (row, d) in entries.into_iter().enumerate() {
                let e = Entry {
                    parent: id,
                    row: row,
                    children: None,
                    data: d,
                };
                children.push(self.entries.len() + row);
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

impl<T: Item> TreeTrait for RGeneralItemModel<T> {
    fn create(emit: TreeEmitter, model: TreeUniformTree) -> Self {
        let none = Entry {
            parent: 0,
            row: 0,
            children: None,
            data: T::default(),
        };
        RGeneralItemModel {
            emit: emit,
            entries: vec![none],
        }
    }
    fn emit(&self) -> &TreeEmitter {
        &self.emit
    }
    fn get_path(&self) -> String {
        String::new()
    }
    fn set_path(&mut self, value: String) {
    }
    fn row_count(&self, row: c_int, parent: usize) -> c_int {
        //let i = self.get(row, parent);
        //self.entries[i].children.as_ref().unwrap().len() as i32
        0
    }
    fn index(&self, row: c_int, parent: usize) -> usize {
        //self.get(row, parent)
        0
    }
    fn parent(&self, row: c_int, index: usize) -> QModelIndex {
        if index < 2 {
            return QModelIndex::invalid();
        }
        let e = &self.entries[index];
        QModelIndex::create(e.row as i32, e.parent)
    }
    fn file_name(&self, row: c_int, parent: usize) -> String {
        //let i = self.get(row, parent);
        //self.entries[i].data.file_name()
        String::new()
    }
    fn file_permissions(&self, row: c_int, parent: usize) -> c_int {
        //let i = self.get(row,parent);
        //self.entries[i].data.file_permissions()
        0
    }
    fn file_icon(&self, row: c_int, parent: usize) -> Vec<u8> {
        Vec::new()
    }
    fn file_path(&self, row: c_int, parent: usize) -> String {
        String::new()
    }
}
