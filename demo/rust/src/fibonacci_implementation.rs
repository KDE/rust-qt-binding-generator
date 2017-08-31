use std::thread;
use fibonacci_interface::*;
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
    fn create(emit: FibonacciEmitter) -> Fibonacci {
        Fibonacci {
            emit: emit,
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
        let emit = self.emit.clone();
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
    model: FibonacciListList,
}

impl FibonacciListTrait for FibonacciList {
    fn create(emit: FibonacciListEmitter, model: FibonacciListList) -> FibonacciList {
        FibonacciList {
            emit: emit,
            model: model,
        }
    }
    fn emit(&self) -> &FibonacciListEmitter {
        &self.emit
    }
    fn row_count(&self) -> usize {
        94
    }
    fn result(&self, row: usize) -> u64 {
        fibonacci(row as u32) as u64
    }
}
