extern crate clap;
extern crate rust_qt_binding_generator;
use clap::{Arg, App};
use rust_qt_binding_generator::*;

fn main() {
    let matches = App::new("rust_qt_binding_generator")
        .version("0.1.0")
        .about("Generates bindings between Qt and Rust")
        .arg(
            Arg::with_name("overwrite-implementation")
                .long("overwrite-implementation")
                .help("Overwrite existing implementation."),
        )
        .arg(
            Arg::with_name("config")
                .multiple(true)
                .required(true)
                .takes_value(true)
                .help("Configuration file(s)"),
        )
        .get_matches();

    let overwrite_implementation = matches.is_present("overwrite-implementation");
    for config in matches.values_of("config").unwrap() {
        if let Err(e) = generate_rust_qt_bindings(config, overwrite_implementation) {
            eprintln!("{}", e);
            ::std::process::exit(1);
        }
    }
}
