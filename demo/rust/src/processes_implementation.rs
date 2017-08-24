use types::*;
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

fn sync_tree(model: &ProcessesUniformTree, parent: Option<usize>,
        avec: &mut Vec<pid_t>,
        amap: &mut HashMap<pid_t, ProcessItem>,
        bvec: &Vec<pid_t>,
        bmap: &mut HashMap<pid_t, ProcessItem>) {
    let mut a = 0;
    let mut b = 0;

    while a < avec.len() && b < bvec.len() {
        if avec[a] < bvec[a] { // a process has disappeared
            let pid = avec[a];
            println!("removing {} '{}' {}", pid, amap[&pid].process.exe,
                amap[&pid].process.cmd.join(" "));
            model.begin_remove_rows(parent, a, a);
            amap.remove(&pid);
            avec.remove(a);
            model.end_remove_rows();
        } else if avec[a] > bvec[b] { // a process has appeared
            let pid = bvec[b];
            println!("adding {} '{}' {}", pid, bmap[&pid].process.exe,
                bmap[&pid].process.cmd.join(" "));
            model.begin_insert_rows(parent, a, a);
            move_process(pid, amap, bmap);
            avec.insert(a, pid);
            let e = amap.get_mut(&pid).unwrap();
            assert_eq!(e.row, a);
            assert_eq!(e.process.parent.map(|p| p as usize), parent);
            model.end_insert_rows();
            a += 1;
            b += 1;
        } else {
            let pid = bvec[b];
            let mut av = amap[&pid].tasks.clone();
            let bv = bmap[&pid].tasks.clone();
            sync_tree(model, Some(pid as usize), &mut av, amap, &bv, bmap);
            let e = amap.get_mut(&pid).unwrap();
            e.tasks = av;
            e.row = a;
            assert_eq!(e.process.parent.map(|p| p as usize), parent);
            a += 1;
            b += 1;
        }
    }
    while a < bvec.len() {
        let pid = bvec[b];
        model.begin_insert_rows(parent, a, a);
        move_process(pid, amap, bmap);
        avec.push(pid);
        assert_eq!(amap.get_mut(&pid).unwrap().row, a);
        model.end_insert_rows();
        a += 1;
        b += 1;
    }
    while b < avec.len() {
        model.begin_remove_rows(parent, a, a);
        amap.remove(&avec[a]);
        avec.remove(a);
        model.end_remove_rows();
    }
    assert_eq!(a, b);
    assert_eq!(a, avec.len());
    assert_eq!(avec.len(), bvec.len());
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
            sync_tree(&self.model, None, &mut self.p.top, &mut self.p.processes,
                &new.top, &mut new.processes);
        }
    }
    fn row(&self, item: usize) -> usize {
        self.get(item).row
    }
    fn pid(&self, item: usize) -> u32 {
        self.get(item).process.pid as u32
    }
    fn uid(&self, item: usize) -> u32 {
        self.get(item).process.uid as u32
    }
    fn cpu_usage(&self, item: usize) -> f32 {
        self.get(item).process.cpu_usage
    }
    fn cpu_percentage(&self, item: usize) -> u8 {
        let cpu = self.get(item).process.cpu_usage / self.p.cpusum;
        (cpu * 100.0) as u8
    }
    fn memory(&self, item: usize) -> u64 {
        self.get(item).process.memory
    }
    fn name(&self, item: usize) -> String {
        self.get(item).process.name.clone()
    }
    fn cmd(&self, item: usize) -> String {
        self.get(item).process.cmd.join(" ")
    }
}
