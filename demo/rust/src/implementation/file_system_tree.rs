// Copyright 2017  Jos van den Oever <jos@vandenoever.info>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License or (at your option) version 3 or any later version
// accepted by the membership of KDE e.V. (or its successor approved
// by the membership of KDE e.V.), which shall act as a proxy
// defined in Section 14 of version 3 of the license.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

use interface::*;
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
    path: Option<PathBuf>,
    icon: Vec<u8>,
}

type Incoming<T> = Arc<Mutex<HashMap<usize, Vec<T>>>>;

impl Item for DirEntry {
    fn new(name: &str) -> DirEntry {
        DirEntry {
            name: OsString::from(name),
            metadata: metadata(name).ok(),
            path: None,
            icon: Vec::new()
        }
    }
    fn can_fetch_more(&self) -> bool {
        self.metadata.as_ref().map_or(false, |m| m.is_dir())
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
        self.metadata.as_ref().map(|m| m.len())
    }
    fn icon(&self) -> &[u8] {
        &self.icon
    }
    fn retrieve(id: usize, parents: Vec<&DirEntry>, q: Incoming<Self>, emit: FileSystemTreeEmitter) {
        let mut v = Vec::new();
        let path: PathBuf = parents.into_iter().map(|e| &e.name).collect();
        thread::spawn(move || {
            if let Ok(it) = read_dir(&path) {
                for i in it.filter_map(|v| v.ok()) {
                    let de = DirEntry {
                        name: i.file_name(),
                        metadata: i.metadata().ok(),
                        path: Some(i.path()),
                        icon: Vec::new(),
                    };
                    v.push(de);
                }
            }
            v.sort_by(|a, b| a.name.cmp(&b.name));
            let mut map = q.lock().unwrap();
            if !map.contains_key(&id) {
                map.insert(id, v);
                emit.new_data_ready(Some(id));
            }
        });
    }
}

impl Default for DirEntry {
    fn default() -> DirEntry {
        DirEntry {
            name: OsString::new(),
            metadata: None,
            path: None,
            icon: Vec::new(),
        }
    }
}

pub trait Item: Default {
    fn new(name: &str) -> Self;
    fn can_fetch_more(&self) -> bool;
    fn retrieve(id: usize, parents: Vec<&Self>, q: Incoming<Self>, emit: FileSystemTreeEmitter);
    fn file_name(&self) -> String;
    fn file_path(&self) -> Option<String>;
    fn file_permissions(&self) -> i32;
    fn file_type(&self) -> i32;
    fn file_size(&self) -> Option<u64>;
    fn icon(&self) -> &[u8];
}

pub type FileSystemTree = RGeneralItemModel<DirEntry>;

struct Entry<T: Item> {
    parent: Option<usize>,
    row: usize,
    children: Option<Vec<usize>>,
    data: T,
}

pub struct RGeneralItemModel<T: Item> {
    emit: FileSystemTreeEmitter,
    model: FileSystemTreeTree,
    entries: Vec<Entry<T>>,
    path: Option<String>,
    incoming: Incoming<T>,
}

impl<T: Item> RGeneralItemModel<T>
where
    T: Sync + Send,
{
    fn reset(&mut self) {
        self.model.begin_reset_model();
        self.entries.clear();
        if let Some(ref path) = self.path {
            let root = Entry {
                parent: None,
                row: 0,
                children: None,
                data: T::new(path),
            };
            self.entries.push(root);
        }
        self.model.end_reset_model();
    }
    fn get(&self, index: usize) -> &Entry<T> {
        &self.entries[index]
    }
    fn retrieve(&mut self, index: usize) {
        let parents = self.get_parents(index);
        let incoming = self.incoming.clone();
        T::retrieve(index, parents, incoming, self.emit.clone());
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
                            parent: Some(id),
                            row: r,
                            children: None,
                            data: d,
                        };
                        children.push(self.entries.len() + r);
                        new_entries.push(e);
                    }
                    if !new_entries.is_empty() {
                        self.model.begin_insert_rows(
                            Some(id),
                            0,
                            (new_entries.len() - 1),
                        );
                    }
                }
                self.entries[id].children = Some(children);
                if !new_entries.is_empty() {
                    self.entries.append(&mut new_entries);
                    self.model.end_insert_rows();
                }
            }
        }
    }
    fn get_parents(&self, id: usize) -> Vec<&T> {
        let mut pos = Some(id);
        let mut e = Vec::new();
        while let Some(p) = pos {
            e.push(p);
            pos = self.entries[p].parent;
        }
        e.into_iter().rev().map(|i| &self.entries[i].data).collect()
    }
}

impl<T: Item> FileSystemTreeTrait for RGeneralItemModel<T>
where
    T: Sync + Send,
{
    fn new(emit: FileSystemTreeEmitter, model: FileSystemTreeTree) -> Self {
        let mut tree = RGeneralItemModel {
            emit: emit,
            model: model,
            entries: Vec::new(),
            path: None,
            incoming: Arc::new(Mutex::new(HashMap::new())),
        };
        tree.reset();
        tree
    }
    fn emit(&self) -> &FileSystemTreeEmitter {
        &self.emit
    }
    fn path(&self) -> Option<&str> {
        self.path.as_ref().map(|s|&s[..])
    }
    fn set_path(&mut self, value: Option<String>) {
        if self.path != value {
            self.path = value;
            self.emit.path_changed();
            self.reset();
        }
    }
    fn can_fetch_more(&self, index: Option<usize>) -> bool {
        if let Some(index) = index {
            let entry = self.get(index);
            entry.children.is_none() && entry.data.can_fetch_more()
        } else {
            false
        }
    }
    fn fetch_more(&mut self, index: Option<usize>) {
        self.process_incoming();
        if !self.can_fetch_more(index) {
            return;
        }
        if let Some(index) = index {
            self.retrieve(index);
        }
    }
    fn row_count(&self, index: Option<usize>) -> usize {
        if self.entries.is_empty() {
            return 0;
        }
        if let Some(i) = index {
            let entry = self.get(i);
            if let Some(ref children) = entry.children {
                children.len()
            } else {
                // model does lazy loading, signal that data may be available
                if self.can_fetch_more(index) {
                    self.emit.new_data_ready(index);
                }
                0
            }
        } else {
            1
        }
    }
    fn index(&self, index: Option<usize>, row: usize) -> usize {
        if let Some(index) = index {
            self.get(index).children.as_ref().unwrap()[row]
        } else {
            0
        }
    }
    fn parent(&self, index: usize) -> Option<usize> {
        self.entries[index].parent
    }
    fn row(&self, index: usize) -> usize {
        self.entries[index].row
    }
    fn file_name(&self, index: usize) -> String {
        self.get(index).data.file_name()
    }
    fn file_permissions(&self, index: usize) -> i32 {
        self.get(index).data.file_permissions()
    }
    #[allow(unused_variables)]
    fn file_icon(&self, index: usize) -> &[u8] {
        self.get(index).data.icon()
    }
    fn file_path(&self, index: usize) -> Option<String> {
        self.get(index).data.file_path()
    }
    fn file_type(&self, index: usize) -> i32 {
        self.get(index).data.file_type()
    }
    fn file_size(&self, index: usize) -> Option<u64> {
        self.get(index).data.file_size()
    }
}
