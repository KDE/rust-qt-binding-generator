extern crate rust_qt_binding_generator;

use rust_qt_binding_generator::build::QtModule;

fn main() {
    let out_dir = ::std::env::var("OUT_DIR").unwrap();
    rust_qt_binding_generator::build::Build::new(&out_dir)
        .bindings("bindings.json")
        .qrc("qml.qrc")
        .cpp("src/main.cpp")
        .module(QtModule::Gui)
        .module(QtModule::Qml)
        .compile("qt_quick_cargo");
}
