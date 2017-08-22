use interface::*;
use types::*;
use std::fs::*;
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
    fn file_permissions(&self) -> i32 {
        42
    }
    fn file_type(&self) -> i32 {
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
                emit.new_data_ready(id);
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
    fn file_permissions(&self) -> i32;
    fn file_type(&self) -> i32;
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
        self.entries.push(Entry {
            parent: 0,
            row: 0,
            children: None,
            data: T::default(),
        });
        if let Some(ref path) = self.path {
            self.entries[0].children = Some(vec![1]);
            let root = Entry {
                parent: 0,
                row: 0,
                children: None,
                data: T::create(&path),
            };
            self.entries.push(root);
        }
        self.model.end_reset_model();
    }
    fn get(&self, item: usize) -> &Entry<T> {
        &self.entries[item]
    }
    fn retrieve(&mut self, item: usize) {
        let parents = self.get_parents(item);
        let incoming = self.incoming.clone();
        T::retrieve(item, parents, incoming, self.emit.clone());
    }
    fn process_incoming(&mut self) {
        if let Ok(ref mut incoming) = self.incoming.try_lock() {
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
                        self.model.begin_insert_rows(id, 0,
                            (new_entries.len() - 1));
                    }
                }
                self.entries[id].children = Some(children);
                if new_entries.len() > 0 {
                    self.entries.append(&mut new_entries);
                    self.model.end_insert_rows();
                }
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
    fn can_fetch_more(&self, item: usize) -> bool {
        let entry = self.get(item);
        entry.children.is_none() && entry.data.can_fetch_more()
    }
    fn fetch_more(&mut self, item: usize) {
        self.process_incoming();
        if !self.can_fetch_more(item) {
            return;
        }
        self.retrieve(item);
    }
    fn row_count(&self, item: usize) -> usize {
        let r = self.get(item).children.as_ref()
            .map(|i| i.len())
            .unwrap_or(0);
        // model does lazy loading, signal that data may be available
        if r == 0 && self.can_fetch_more(item) {
            self.emit.new_data_ready(item);
        }
        r
    }
    fn index(&self, item: usize, row: usize) -> usize {
        self.entries.get(item)
            .and_then(|i| i.children.as_ref())
            .and_then(|i| i.get(row))
            .map(|i| *i)
            .unwrap_or(0)
    }
    fn parent(&self, item: usize) -> QModelIndex {
        if item >= self.entries.len() || item == 0 {
            return QModelIndex::invalid();
        }
        let entry = &self.entries[item];
        QModelIndex::create(entry.row as i32, entry.parent)
    }
    fn file_name(&self, item: usize) -> String {
        self.get(item).data.file_name()
    }
    fn file_permissions(&self, item: usize) -> i32 {
        self.get(item).data.file_permissions()
    }
    #[allow(unused_variables)]
    fn file_icon(&self, item: usize) -> Vec<u8> {
        Vec::new()
    }
    fn file_path(&self, item: usize) -> Option<String> {
        self.get(item).data.file_path()
    }
    fn file_type(&self, item: usize) -> i32 {
        self.get(item).data.file_type()
    }
    fn file_size(&self, item: usize) -> Option<u64> {
        self.get(item).data.file_size()
    }
}
