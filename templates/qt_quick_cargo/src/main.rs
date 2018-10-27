extern crate libc;

mod implementation;
pub mod interface {
    include!(concat!(env!("OUT_DIR"), "/src/interface.rs"));
}

use std::os::raw::c_char;
extern {
    fn main_cpp(app: *const c_char);
}

fn main() {
    use std::ffi::CString;
    let mut args = ::std::env::args();
    let app = CString::new(args.next().unwrap()).unwrap();
    unsafe {
        main_cpp(app.as_ptr());
    }
}
