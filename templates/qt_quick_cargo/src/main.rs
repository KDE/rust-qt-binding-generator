extern crate libc;

mod implementation;
pub mod interface {
    include!(concat!(env!("OUT_DIR"), "/src/interface.rs"));
}

extern {
    fn main_cpp(app: *const ::std::os::raw::c_char);
}

fn main() {
    use std::ffi::CString;
    let app_name = ::std::env::args().next().unwrap();
    let app_name = CString::new(app_name).unwrap();
    unsafe {
        main_cpp(app_name.as_ptr());
    }
}
