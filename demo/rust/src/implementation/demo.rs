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
