use interface::*;
use types::*;
use std::fs::*;
use libc::{c_int};
use std::fs::read_dir;
use std::path::PathBuf;
use std::ffi::OsString;
use std::default::Default;
use std::thread;
use std::sync::{Arc, Mutex};
use std::marker::Sync;
use std::collections::HashMap;

pub struct DirEntry {
    name: OsString,
    metadata: Option<Metadata>,
    path: Option<PathBuf>
}

type Incoming<T> = Arc<Mutex<HashMap<usize, Vec<T>>>>;

impl Item for DirEntry {
    fn create(name: &str) -> DirEntry {
        DirEntry {
            name: OsString::from(name),
            metadata: metadata(name).ok(),
            path: None
        }
    }
    fn can_fetch_more(&self) -> bool {
        self.metadata.as_ref().map_or(false, |m|m.is_dir())
    }
    fn file_name(&self) -> String {
        self.name.to_string_lossy().to_string()
    }
    fn file_path(&self) -> Option<String> {
        self.path.as_ref().map(|p| p.to_string_lossy().into())
    }
    fn file_permissions(&self) -> c_int {
        42
    }
    fn file_type(&self) -> c_int {
        0
    }
    fn file_size(&self) -> Option<u64> {
        self.metadata.as_ref().map(|m|m.len())
    }
    fn retrieve(id: usize, parents: Vec<&DirEntry>,
            q: Incoming<Self>,
            emit: TreeEmitter) {
        let mut v = Vec::new();
        let path: PathBuf = parents.into_iter().map(|e| &e.name).collect();
        thread::spawn(move || {
            if let Ok(it) = read_dir(&path) {
                for i in it.filter_map(|v| v.ok()) {
                    let de = DirEntry {
                        name: i.file_name(),
                        metadata: i.metadata().ok(),
                        path: Some(i.path())
                    };
                    v.push(de);
                }
            }
            v.sort_by(|a, b| a.name.cmp(&b.name));
            let mut map = q.lock().unwrap();
            if !map.contains_key(&id) {
                map.insert(id, v);
                emit.new_data_ready(0, 0);
            }
        });
    }
}

impl Default for DirEntry {
    fn default() -> DirEntry {
        DirEntry {
            name: OsString::new(),
            metadata: None,
            path: None
        }
    }
}

pub trait Item: Default {
    fn create(name: &str) -> Self;
    fn can_fetch_more(&self) -> bool;
    fn retrieve(id: usize, parents: Vec<&Self>, q: Incoming<Self>, emit: TreeEmitter);
    fn file_name(&self) -> String;
    fn file_path(&self) -> Option<String>;
    fn file_permissions(&self) -> c_int;
    fn file_type(&self) -> c_int;
    fn file_size(&self) -> Option<u64>;
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
    path: Option<String>,
    incoming: Incoming<T>,
}

impl<T: Item> RGeneralItemModel<T> where T: Sync + Send {
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
            children: None,
            data: T::default(),
        };
        self.entries.push(none1);
        if let Some(ref path) = self.path {
            self.entries[1].children = Some(vec![2]);
            let root = Entry {
                parent: 1,
                row: 0,
                children: None,
                data: T::create(&path),
            };
            self.entries.push(root);
        }
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
    fn get_mut(&mut self, row: c_int, parent: usize) -> Option<&mut Entry<T>> {
        self.get_index(row, parent)
            .map(move |i| &mut self.entries[i])
    }
    fn retrieve(&mut self, row: c_int, parent: usize) {
        let id = self.get_index(row, parent).unwrap();
        let parents = self.get_parents(id);
        let incoming = self.incoming.clone();
        T::retrieve(id, parents, incoming, self.emit.clone());
    }
    fn process_incoming(&mut self) {
        let mut incoming = self.incoming.lock().unwrap();
        for (id, entries) in incoming.drain() {
            if self.entries[id].children.is_some() {
                continue;
            }
            let mut new_entries = Vec::new();
            let mut children = Vec::new();
            {
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
                    let ref e = self.entries[id];
                    self.model.begin_insert_rows(e.row as c_int, e.parent, 0,
                        (new_entries.len() - 1) as c_int);
                }
            }
            self.entries[id].children = Some(children);
            if new_entries.len() > 0 {
                self.entries.append(&mut new_entries);
                self.model.end_insert_rows();
            }
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

impl<T: Item> TreeTrait for RGeneralItemModel<T> where T: Sync + Send {
    fn create(emit: TreeEmitter, model: TreeUniformTree) -> Self {
        let mut tree = RGeneralItemModel {
            emit: emit,
            model: model,
            entries: Vec::new(),
            path: None,
            incoming: Arc::new(Mutex::new(HashMap::new()))
        };
        tree.reset();
        tree
    }
    fn emit(&self) -> &TreeEmitter {
        &self.emit
    }
    fn get_path(&self) -> Option<String> {
        self.path.clone()
    }
    fn set_path(&mut self, value: Option<String>) {
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
        self.process_incoming();
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
    fn file_path(&self, row: c_int, parent: usize) -> Option<String> {
        self.get(row, parent)
            .map(|entry| entry.data.file_path())
            .unwrap_or_default()
    }
    fn file_type(&self, row: c_int, parent: usize) -> c_int {
        self.get(row, parent)
            .map(|entry| entry.data.file_type())
            .unwrap_or_default()
    }
    fn file_size(&self, row: c_int, parent: usize) -> Option<u64> {
        self.get(row, parent)
            .map(|entry| entry.data.file_size())
            .unwrap_or(None)
    }
}
