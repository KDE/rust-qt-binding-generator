use interface::*;
use types::*;
use std::fs::*;
use libc::{c_int, c_ulonglong};
use std::fs::read_dir;
use std::path::PathBuf;
use std::ffi::OsString;
use std::default::Default;
use std::thread;

pub struct DirEntry {
    name: OsString,
    metadata: Option<Metadata>,
}

impl Item for DirEntry {
    fn create(name: &str) -> DirEntry {
        DirEntry {
            name: OsString::from(name),
            metadata: metadata(name).ok(),
        }
    }
    fn can_fetch_more(&self) -> bool {
        self.metadata.as_ref().map_or(false, |m|m.is_dir())
    }
    fn file_name(&self) -> String {
        self.name.to_string_lossy().to_string()
    }
    fn file_permissions(&self) -> c_int {
        42
    }
    fn file_type(&self) -> c_int {
        0
    }
    fn file_size(&self) -> u64 {
        self.metadata.as_ref().map_or(0, |m|m.len())
    }
    fn retrieve(&self, parents: Vec<&DirEntry>) -> Vec<DirEntry> {
        let mut v = Vec::new();
        let path: PathBuf = parents.into_iter().map(|e| &e.name).collect();
        if let Ok(it) = read_dir(path) {
            for i in it.filter_map(|v| v.ok()) {
                let de = DirEntry {
                    name: i.file_name(),
                    metadata: i.metadata().ok()
                };
                v.push(de);
            }
        }
        v.sort_by(|a, b| a.name.cmp(&b.name));
        v
    }
}

impl Default for DirEntry {
    fn default() -> DirEntry {
        DirEntry {
            name: OsString::new(),
            metadata: None,
        }
    }
}

pub trait Item: Default {
    fn create(name: &str) -> Self;
    fn can_fetch_more(&self) -> bool;
    fn retrieve(&self, parents: Vec<&Self>) -> Vec<Self>;
    fn file_name(&self) -> String;
    fn file_permissions(&self) -> c_int;
    fn file_type(&self) -> c_int;
    fn file_size(&self) -> u64;
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
    model: TreeUniformTree,
    entries: Vec<Entry<T>>,
    path: String,
}

impl<T: Item> RGeneralItemModel<T> {
    fn reset(&mut self) {
        self.model.begin_reset_model();
        self.entries.clear();
        let none0 = Entry {
            parent: 0,
            row: 0,
            children: Some(vec![1]),
            data: T::default(),
        };
        self.entries.push(none0);
        let none1 = Entry {
            parent: 0,
            row: 0,
            children: Some(vec![2]),
            data: T::default(),
        };
        self.entries.push(none1);
        let root = Entry {
            parent: 1,
            row: 0,
            children: None,
            data: T::create(&self.path),
        };
        self.entries.push(root);
        self.model.end_reset_model();
    }
    fn get_index(&self, row: c_int, parent: usize) -> Option<usize> {
        // for an invalid index return the root
        if parent == 0 || row < 0 {
            return Some(1);
        }
        self.entries.get(parent)
            .and_then(|i| i.children.as_ref())
            .and_then(|i| i.get(row as usize))
            .map(|i| *i)
    }
    fn get(&self, row: c_int, parent: usize) -> Option<&Entry<T>> {
        self.get_index(row, parent)
            .map(|i| &self.entries[i])
    }
    fn retrieve(&mut self, row: c_int, parent: usize) {
        let id = self.get_index(row, parent).unwrap();
        let mut new_entries = Vec::new();
        let mut children = Vec::new();
        {
            let parents = self.get_parents(id);
            let entry = &self.entries[id];
            let entries = entry.data.retrieve(parents);
            for (r, d) in entries.into_iter().enumerate() {
                let e = Entry {
                    parent: id,
                    row: r,
                    children: None,
                    data: d,
                };
                children.push(self.entries.len() + r);
                new_entries.push(e);
            }
            if new_entries.len() > 0 {
                self.model.begin_insert_rows(row, parent, 0,
                    (new_entries.len() - 1) as c_int);
            }
        }
        self.entries[id].children = Some(children);
        if new_entries.len() > 0 {
            self.entries.append(&mut new_entries);
            self.model.end_insert_rows();
        }
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
        let mut tree = RGeneralItemModel {
            emit: emit,
            model: model,
            entries: Vec::new(),
            path: String::new()
        };
        tree.reset();
        tree
    }
    fn emit(&self) -> &TreeEmitter {
        &self.emit
    }
    fn get_path(&self) -> String {
        self.path.clone()
    }
    fn set_path(&mut self, value: String) {
        if self.path != value {
            self.path = value;
            self.emit.path_changed();
            self.reset();
        }
    }
    fn can_fetch_more(&self, row: c_int, parent: usize) -> bool {
        self.get(row, parent)
            .map(|entry| entry.children.is_none()
                 && entry.data.can_fetch_more())
            .unwrap_or(false)
    }
    fn fetch_more(&mut self, row: c_int, parent: usize) {
        if !self.can_fetch_more(row, parent) {
            return;
        }
        self.retrieve(row, parent);
    }
    fn row_count(&self, row: c_int, parent: usize) -> c_int {
        let r = self.get(row, parent)
            .and_then(|entry| entry.children.as_ref())
            .map(|i| i.len())
            .unwrap_or(0) as c_int;
        // model does lazy loading, signal that data may be available
        if r == 0 && self.can_fetch_more(row, parent) {
            self.emit.new_data_ready(row, parent);
        }
        r
    }
    fn index(&self, row: c_int, parent: usize) -> usize {
        self.get_index(row, parent).unwrap_or(0)
    }
    fn parent(&self, index: usize) -> QModelIndex {
        if index >= self.entries.len() {
            return QModelIndex::invalid();
        }
        let entry = &self.entries[index];
        QModelIndex::create(entry.row as i32, entry.parent)
    }
    fn file_name(&self, row: c_int, parent: usize) -> String {
        self.get(row, parent)
            .map(|entry| entry.data.file_name())
            .unwrap_or_default()
    }
    fn file_permissions(&self, row: c_int, parent: usize) -> c_int {
        self.get(row, parent)
            .map(|entry| entry.data.file_permissions())
            .unwrap_or_default()
    }
    fn file_icon(&self, row: c_int, parent: usize) -> Vec<u8> {
        Vec::new()
    }
    fn file_path(&self, row: c_int, parent: usize) -> String {
        String::new()
    }
    fn file_type(&self, row: c_int, parent: usize) -> c_int {
        self.get(row, parent)
            .map(|entry| entry.data.file_type())
            .unwrap_or_default()
    }
    fn file_size(&self, row: c_int, parent: usize) -> c_ulonglong {
        self.get(row, parent)
            .map(|entry| entry.data.file_size())
            .unwrap_or_default()
    }
}
