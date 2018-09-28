extern crate regex;
#[macro_use]
extern crate serde_derive;
extern crate serde_json;

mod configuration;
mod cpp;
mod rust;
mod util;

use std::error::Error;
use std::fmt::Display;
use std::path::Path;

pub fn generate_rust_qt_bindings<P: AsRef<Path> + Display>(
    config_file: P,
    overwrite_implementation: bool,
) -> Result<(), Box<Error>> {
    let mut config = configuration::parse(config_file)?;
    if overwrite_implementation {
        config.overwrite_implementation = true;
    }
    cpp::write_header(&config)?;
    cpp::write_cpp(&config)?;
    rust::write_interface(&config)?;
    rust::write_implementation(&config)?;
    Ok(())
}
