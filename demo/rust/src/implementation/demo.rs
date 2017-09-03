use interface::*;
use super::*;

pub struct Demo {
    emit: DemoEmitter,
    fibonacci: Fibonacci,
    fibonacci_list: FibonacciList,
    file_system_tree: FileSystemTree,
    processes: Processes,
    time_series: TimeSeries
}

impl DemoTrait for Demo {
    fn new(emit: DemoEmitter,
        fibonacci: Fibonacci,
        fibonacci_list: FibonacciList,
        file_system_tree: FileSystemTree,
        processes: Processes,
        time_series: TimeSeries) -> Self {
        Demo {
            emit: emit,
            fibonacci: fibonacci,
            fibonacci_list: fibonacci_list,
            file_system_tree: file_system_tree,
            processes: processes,
            time_series: time_series
        }
    }
    fn emit(&self) -> &DemoEmitter {
        &self.emit
    }
    fn fibonacci(&self) -> &Fibonacci {
        &self.fibonacci
    }
    fn fibonacci_mut(&mut self) -> &mut Fibonacci {
        &mut self.fibonacci
    }
    fn fibonacci_list(&self) -> &FibonacciList {
        &self.fibonacci_list
    }
    fn fibonacci_list_mut(&mut self) -> &mut FibonacciList {
        &mut self.fibonacci_list
    }
    fn file_system_tree(&self) -> &FileSystemTree {
        &self.file_system_tree
    }
    fn file_system_tree_mut(&mut self) -> &mut FileSystemTree {
        &mut self.file_system_tree
    }
    fn processes(&self) -> &Processes {
        &self.processes
    }
    fn processes_mut(&mut self) -> &mut Processes {
        &mut self.processes
    }
    fn time_series(&self) -> &TimeSeries {
        &self.time_series
    }
    fn time_series_mut(&mut self) -> &mut TimeSeries {
        &mut self.time_series
    }
}
