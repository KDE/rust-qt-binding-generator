extern crate regex;
#[macro_use]
extern crate serde_derive;
extern crate serde_json;

pub mod configuration;
mod configuration_private;
mod cpp;
mod rust;
mod util;

use std::error::Error;
use std::fmt::Display;
use std::path::Path;
use configuration::Config;

pub fn generate_bindings(config: &Config) -> Result<(), Box<Error>> {
    cpp::write_header(config)?;
    cpp::write_cpp(config)?;
    rust::write_interface(config)?;
    rust::write_implementation(config)?;
    Ok(())
}

pub fn generate_bindings_from_config_file<P: AsRef<Path> + Display>(
    config_file: P,
    overwrite_implementation: bool,
) -> Result<(), Box<Error>> {
    let mut config = configuration::parse(config_file)?;
    if overwrite_implementation {
        config.overwrite_implementation = true;
    }
    generate_bindings(&config)
}
