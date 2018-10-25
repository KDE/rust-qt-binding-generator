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

use std::thread;
use interface::*;
use std::sync::atomic::AtomicUsize;
use std::sync::atomic::Ordering;
use std::sync::Arc;

fn fibonacci(input: u32) -> usize {
    if input <= 1 {
        return input as usize;
    }
    let mut i = 0;
    let mut sum = 0;
    let mut last = 0;
    let mut cur = 1;
    while i < input - 1 {
        sum = last + cur;
        last = cur;
        cur = sum;
        i += 1;
    }
    sum
}

pub struct Fibonacci {
    emit: FibonacciEmitter,
    input: u32,
    result: Arc<AtomicUsize>,
}

impl FibonacciTrait for Fibonacci {
    fn new(emit: FibonacciEmitter) -> Fibonacci {
        Fibonacci {
            emit,
            input: 0,
            result: Arc::new(AtomicUsize::new(0)),
        }
    }
    fn emit(&self) -> &FibonacciEmitter {
        &self.emit
    }
    fn input(&self) -> u32 {
        self.input
    }
    fn set_input(&mut self, value: u32) {
        self.input = value;
        self.emit.input_changed();
        let mut emit = self.emit.clone();
        let result = self.result.clone();
        result.swap(0, Ordering::SeqCst);
        emit.result_changed();
        thread::spawn(move || {
            let r = fibonacci(value);
            result.swap(r, Ordering::SeqCst);
            emit.result_changed();
        });
    }
    fn result(&self) -> u64 {
        self.result.fetch_add(0, Ordering::SeqCst) as u64
    }
}

pub struct FibonacciList {
    emit: FibonacciListEmitter,
}

impl FibonacciListTrait for FibonacciList {
    fn new(emit: FibonacciListEmitter, _: FibonacciListList) -> FibonacciList {
        FibonacciList {
            emit,
        }
    }
    fn emit(&self) -> &FibonacciListEmitter {
        &self.emit
    }
    fn row_count(&self) -> usize {
        93
    }
    fn row(&self, row: usize) -> u64 {
        row as u64 + 1
    }
    fn fibonacci_number(&self, row: usize) -> u64 {
        fibonacci(row as u32 + 1) as u64
    }
}
