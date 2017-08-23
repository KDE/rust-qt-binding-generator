use types::*;
use processes_interface::*;
use sysinfo::*;
use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use libc::pid_t;
use std::{thread, time};

struct ProcessItem {
    parent: Option<usize>,
    row: usize,
    tasks: Vec<usize>,
    process: Process
}

#[derive (Default)]
struct ProcessTree {
    top: Vec<usize>,
    processes: Vec<ProcessItem>,
    cpusum: f32
}

pub struct Processes {
    emit: ProcessesEmitter,
    model: ProcessesUniformTree,
    p: ProcessTree,
    incoming: Arc<Mutex<Option<ProcessTree>>>
}

fn handle_tasks(tasks: &HashMap<pid_t,Process>,
        mut processes: &mut Vec<ProcessItem>,
        parentPos: Option<usize>) -> (Vec<usize>, f32) {
    let mut t = Vec::new();
    let mut i = 0;
    let mut cpusum = 0.0;
    for process in tasks.values() {
        let pos = processes.len();
        t.push(pos);
        processes.push(ProcessItem {
            parent: parentPos,
            row: i,
            tasks: Vec::new(),
            process: process.clone()
        });
        let (t, s) = handle_tasks(&process.tasks, &mut processes, Some(pos));
        processes[pos].tasks = t;
        cpusum += process.cpu_usage + s;
        i += 1;
    }
    (t, cpusum)
}

fn update() -> ProcessTree {
    let mut p = ProcessTree::default();
    let mut sysinfo = System::new();
    sysinfo.refresh_processes();
    let (top, cpusum) = handle_tasks(sysinfo.get_process_list(),
        &mut p.processes, None);
    p.top = top;
    p.cpusum = cpusum;
    p
}

fn update_thread(emit: ProcessesEmitter, incoming: Arc<Mutex<Option<ProcessTree>>>) {
    thread::spawn(move || {
        let second = time::Duration::new(1, 0);
        while true {
            *incoming.lock().unwrap() = Some(update());
            emit.new_data_ready(None);
            thread::sleep(second);
        }
    });
}

impl ProcessesTrait for Processes {
    fn create(emit: ProcessesEmitter, model: ProcessesUniformTree) -> Processes {
        let mut p = Processes {
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
            self.p.processes[item].tasks.len()
        } else {
            self.p.top.len()
        }
    }
    fn index(&self, item: Option<usize>, row: usize) -> usize {
        if let Some(item) = item {
            self.p.processes[item].tasks[row]
        } else {
            self.p.top[row]
        }
    }
    fn parent(&self, item: usize) -> Option<usize> {
        self.p.processes[item].parent
    }
    fn can_fetch_more(&self, item: Option<usize>) -> bool {
        if let Ok(ref incoming) = self.incoming.try_lock() {
            incoming.is_some()
        } else {
            false
        }
    }
    fn fetch_more(&mut self, item: Option<usize>) {
        if let Ok(ref mut incoming) = self.incoming.try_lock() {
            if let Some(more) = incoming.take() {
                self.model.begin_reset_model();
                self.p = more;
                self.model.end_reset_model();
            }
        }
    }
    fn row(&self, item: usize) -> usize {
        self.p.processes[item].row
    }
    fn pid(&self, item: usize) -> u32 {
        self.p.processes[item].process.pid as u32
    }
    fn uid(&self, item: usize) -> u32 {
        self.p.processes[item].process.uid as u32
    }
    fn cpu_usage(&self, item: usize) -> f32 {
        self.p.processes[item].process.cpu_usage
    }
    fn cpu_percentage(&self, item: usize) -> u8 {
        let cpu = self.p.processes[item].process.cpu_usage / self.p.cpusum;
        (cpu * 100.0) as u8
    }
    fn memory(&self, item: usize) -> u64 {
        self.p.processes[item].process.memory
    }
    fn name(&self, item: usize) -> String {
        self.p.processes[item].process.name.clone()
    }
    fn cmd(&self, item: usize) -> String {
        self.p.processes[item].process.cmd.join(" ")
    }
}
