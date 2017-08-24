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

fn check() {
    fn check_process_hierarchy(parent: Option<pid_t>, processes: &HashMap<pid_t,Process>) {
        for (pid, process) in processes {
            assert_eq!(process.pid, *pid);
            if !parent.is_none() {
                assert_eq!(process.parent, parent);
            }
            check_process_hierarchy(Some(*pid), &process.tasks);
        }
    }

    let mut sysinfo = System::new();
    sysinfo.refresh_processes();
    check_process_hierarchy(None, sysinfo.get_process_list());
}

fn collect_processes(tasks: &HashMap<pid_t,Process>,
        mut processes: &mut HashMap<pid_t, ProcessItem>) -> f32 {
    let mut cpusum = 0.0;
    for (pid, process) in tasks {
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
            let row = {
                let p = processes.get_mut(&parent).unwrap();
                let row = p.tasks.len();
                p.tasks.push(pid);
                row
            };
            processes.get_mut(&pid).unwrap().row = row;
        } else {
            top.push(pid);
        }
    }
    top.sort();
    top
}

fn sort_tasks(mut processes: &mut HashMap<pid_t, ProcessItem>) {
    for process in processes.values_mut() {
        process.tasks.sort();
    }
}

fn update() -> ProcessTree {
    check();
    let mut p = ProcessTree::default();
    let mut sysinfo = System::new();
    sysinfo.refresh_processes();
    p.cpusum = collect_processes(sysinfo.get_process_list(), &mut p.processes);
    p.top = handle_tasks(&mut p.processes);
    sort_tasks(&mut p.processes);
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

impl Processes {
    fn get(&self, item: usize) -> &ProcessItem {
        let pid = item as pid_t;
        &self.p.processes[&pid]
    }
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
