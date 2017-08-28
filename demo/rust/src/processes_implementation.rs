use processes_interface::*;
use sysinfo::*;
use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use libc::pid_t;
use std::{thread, time};

struct ProcessItem {
    row: usize,
    tasks: Vec<pid_t>,
    process: Process
}

#[derive (Default)]
struct ProcessTree {
    top: Vec<pid_t>,
    processes: HashMap<pid_t, ProcessItem>,
    cpusum: f32
}

pub struct Processes {
    emit: ProcessesEmitter,
    model: ProcessesUniformTree,
    p: ProcessTree,
    incoming: Arc<Mutex<Option<ProcessTree>>>
}

fn check_process_hierarchy(parent: Option<pid_t>, processes: &HashMap<pid_t,Process>) {
    for (pid, process) in processes {
        assert_eq!(process.pid, *pid);
        if !parent.is_none() {
            assert_eq!(process.parent, parent);
        }
        check_process_hierarchy(Some(*pid), &process.tasks);
    }
}

fn collect_processes(tasks: &HashMap<pid_t,Process>,
        mut processes: &mut HashMap<pid_t, ProcessItem>) -> f32 {
    let mut cpusum = 0.0;
    for process in tasks.values() {
        processes.insert(process.pid, ProcessItem {
            row: 0,
            tasks: Vec::new(),
            process: process.clone()
        });
        let s = collect_processes(&process.tasks, &mut processes);
        cpusum += process.cpu_usage + s;
    }
    cpusum
}

// reconstruct process hierarchy
fn handle_tasks(mut processes: &mut HashMap<pid_t, ProcessItem>) -> Vec<pid_t> {
    let mut top = Vec::new();
    let pids: Vec<pid_t> = processes.keys().map(|p| *p).collect();
    for pid in pids {
        if let Some(parent) = processes[&pid].process.parent {
            let p = processes.get_mut(&parent).unwrap();
            p.tasks.push(pid);
        } else {
            top.push(pid);
        }
    }
    top
}

fn update_rows(list: Vec<pid_t>, mut processes: &mut HashMap<pid_t, ProcessItem>) {
    let mut row = 0;
    for pid in list {
        processes.get_mut(&pid).unwrap().row = row;
        let l = processes[&pid].tasks.clone();
        update_rows(l, processes);
        row += 1;
    }
}

fn sort_tasks(p: &mut ProcessTree) {
    for process in p.processes.values_mut() {
        process.tasks.sort();
    }
    p.top.sort();
    update_rows(p.top.clone(), &mut p.processes);
}

fn update() -> ProcessTree {
    let mut p = ProcessTree::default();
    let mut sysinfo = System::new();
    sysinfo.refresh_processes();
    let list = sysinfo.get_process_list();
    check_process_hierarchy(None, list);
    p.cpusum = collect_processes(list, &mut p.processes);
    p.top = handle_tasks(&mut p.processes);
    sort_tasks(&mut p);
    p
}

fn update_thread(emit: ProcessesEmitter, incoming: Arc<Mutex<Option<ProcessTree>>>) {
    thread::spawn(move || {
        let second = time::Duration::new(1, 0);
        loop {
            *incoming.lock().unwrap() = Some(update());
            emit.new_data_ready(None);
            thread::sleep(second);
        }
    });
}

impl Processes {
    fn get(&self, item: usize) -> &ProcessItem {
        let pid = item as pid_t;
        &self.p.processes[&pid]
    }
    fn process(&self, item: usize) -> &Process {
        let pid = item as pid_t;
        &self.p.processes[&pid].process
    }
}

fn move_process(pid: pid_t, amap: &mut HashMap<pid_t, ProcessItem>,
        bmap: &mut HashMap<pid_t, ProcessItem>) {
    if let Some(e) = bmap.remove(&pid) {
        amap.insert(pid, e);
        let ts = amap[&pid].tasks.clone();
        for t in ts {
            move_process(t, amap, bmap);
        }
    }
}

fn remove_row(model: &ProcessesUniformTree, parent: pid_t, row: usize,
        map: &mut HashMap<pid_t, ProcessItem>) {
    let pid = map[&parent].tasks[row];
    println!("removing {} '{}' {}", pid, map[&pid].process.exe,
                map[&pid].process.cmd.join(" "));
    model.begin_remove_rows(Some(parent as usize), row, row);
    map.remove(&pid);
    let len = {
        let ref mut tasks = map.get_mut(&parent).unwrap().tasks;
        tasks.remove(row);
        tasks.len()
    };
    for r in row..len {
        let pid = map.get_mut(&parent).unwrap().tasks[r];
        map.get_mut(&pid).unwrap().row = r;
    }
    model.end_remove_rows();
}

fn insert_row(model:  &ProcessesUniformTree, parent: pid_t, row: usize,
        map: &mut HashMap<pid_t, ProcessItem>, pid: pid_t,
        source: &mut HashMap<pid_t, ProcessItem>) {
    println!("adding {} '{}' {}", pid, source[&pid].process.exe,
            source[&pid].process.cmd.join(" "));
    model.begin_insert_rows(Some(parent as usize), row, row);
    move_process(pid, map, source);
    let len = {
        let ref mut tasks = map.get_mut(&parent).unwrap().tasks;
        tasks.insert(row, pid);
        tasks.len()
    };
    for r in row..len {
        let pid = map.get_mut(&parent).unwrap().tasks[r];
        map.get_mut(&pid).unwrap().row = r;
    }
    model.end_insert_rows();
}

fn sync_row(model: &ProcessesUniformTree, pid: pid_t,
        a: &mut Process, b: &Process) {
    let mut changed = false;
    if a.name != b.name {
        a.name.clone_from(&b.name);
        changed = true;
    }
    if a.cpu_usage != b.cpu_usage {
        a.cpu_usage = b.cpu_usage;
        changed = true;
    }
    if a.cmd != b.cmd {
        a.cmd.clone_from(&b.cmd);
        changed = true;
    }
    if a.exe != b.exe {
        a.exe.clone_from(&b.exe);
        changed = true;
    }
    if a.memory != b.memory {
        a.memory = b.memory;
        changed = true;
    }
    if changed {
        model.data_changed(pid as usize, pid as usize);
    }
}

fn sync_tree(model: &ProcessesUniformTree, parent: pid_t,
        amap: &mut HashMap<pid_t, ProcessItem>,
        bmap: &mut HashMap<pid_t, ProcessItem>) {
    let mut a = 0;
    let mut b = 0;
    let mut alen = amap[&parent].tasks.len();
    let blen = bmap[&parent].tasks.len();

    while a < alen && b < blen {
        let apid = amap[&parent].tasks[a];
        let bpid = bmap[&parent].tasks[b];
        if apid < bpid { // a process has disappeared
            remove_row(model, parent, a, amap);
            alen -= 1;
        } else if apid > bpid { // a process has appeared
            insert_row(model, parent, a, amap, bpid, bmap);
            a += 1;
            alen += 1;
            b += 1;
        } else {
            sync_row(model, apid,  &mut amap.get_mut(&apid).unwrap().process,
                    &bmap[&apid].process);
            sync_tree(model, apid, amap, bmap);
            a += 1;
            b += 1;
        }
    }
    while a < blen {
        let bpid = bmap[&parent].tasks[b];
        insert_row(model, parent, a, amap, bpid, bmap);
        a += 1;
        alen += 1;
        b += 1;
    }
    while b < alen {
        remove_row(model, parent, a, amap);
        alen -= 1;
    }
    assert_eq!(a, b);
}

impl ProcessesTrait for Processes {
    fn create(emit: ProcessesEmitter, model: ProcessesUniformTree) -> Processes {
        let p = Processes {
            emit: emit.clone(),
            model: model,
            p: ProcessTree::default(),
            incoming: Arc::new(Mutex::new(None))
        };
        update_thread(emit, p.incoming.clone());
        p
    }
    fn emit(&self) -> &ProcessesEmitter {
        &self.emit
    }
    fn row_count(&self, item: Option<usize>) -> usize {
        if let Some(item) = item {
            self.get(item).tasks.len()
        } else {
            self.p.top.len()
        }
    }
    fn index(&self, item: Option<usize>, row: usize) -> usize {
        if let Some(item) = item {
            self.get(item).tasks[row] as usize
        } else {
            self.p.top[row] as usize
        }
    }
    fn parent(&self, item: usize) -> Option<usize> {
        self.get(item).process.parent.map(|pid| pid as usize)
    }
    fn can_fetch_more(&self, item: Option<usize>) -> bool {
        if item.is_some() {
            return false;
        }
        if let Ok(ref incoming) = self.incoming.try_lock() {
            incoming.is_some()
        } else {
            false
        }
    }
    fn fetch_more(&mut self, item: Option<usize>) {
        if item.is_some() {
            return;
        }
        let new = if let Ok(ref mut incoming) = self.incoming.try_lock() {
            incoming.take()
        } else {
            None
        };
        if let Some(mut new) = new {
            // alert! at the top level, only adding is supported!
            if self.p.top.len() == 0 {
                self.model.begin_reset_model();
                self.p = new;
                self.model.end_reset_model();
            } else {
                let top = self.p.top.clone();
                for pid in top {
                    sync_tree(&self.model, pid, &mut self.p.processes,
                        &mut new.processes);
                }
            }
        }
    }
    fn row(&self, item: usize) -> usize {
        self.get(item).row
    }
    fn pid(&self, item: usize) -> u32 {
        self.process(item).pid as u32
    }
    fn uid(&self, item: usize) -> u32 {
        self.process(item).uid as u32
    }
    fn cpu_usage(&self, item: usize) -> f32 {
        self.process(item).cpu_usage
    }
    fn cpu_percentage(&self, item: usize) -> u8 {
        let cpu = self.process(item).cpu_usage / self.p.cpusum;
        (cpu * 100.0) as u8
    }
    fn memory(&self, item: usize) -> u64 {
        self.process(item).memory
    }
    fn name(&self, item: usize) -> String {
        self.process(item).name.clone()
    }
    fn cmd(&self, item: usize) -> String {
        self.process(item).cmd.join(" ")
    }
}
