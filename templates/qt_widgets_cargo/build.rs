extern crate rust_qt_binding_generator;

use rust_qt_binding_generator::build::QtModule;

fn main() {
    let out_dir = ::std::env::var("OUT_DIR").unwrap();
    rust_qt_binding_generator::build::Build::new(&out_dir)
        .bindings("bindings.json")
        .ui("src/main.ui")
        .cpp("src/main.cpp")
        .module(QtModule::Gui)
        .module(QtModule::Widgets)
        .compile("qt_widgets_cargo");
}
