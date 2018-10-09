extern crate regex;
#[macro_use]
extern crate serde_derive;
extern crate serde_json;
extern crate serde_xml_rs;

pub mod build;
pub mod configuration;
mod configuration_private;
mod cpp;
mod rust;
mod util;

use std::error::Error;
use std::path::Path;
use configuration::Config;

/// Read a file with bindings.
pub fn read_bindings_file<P: AsRef<Path>>(
    config_file: P,
) -> Result<Config, Box<Error>> {
    configuration::parse(config_file)
}

/// Generate bindings from a bindings configuration.
pub fn generate_bindings(config: &Config) -> Result<(), Box<Error>> {
    cpp::write_header(config)?;
    cpp::write_cpp(config)?;
    rust::write_interface(config)?;
    rust::write_implementation(config)?;
    Ok(())
}

/// Generate bindings from a bindings configuration file.
pub fn generate_bindings_from_config_file<P: AsRef<Path>>(
    config_file: P,
    overwrite_implementation: bool,
) -> Result<(), Box<Error>> {
    let mut config = read_bindings_file(config_file)?;
    if overwrite_implementation {
        config.overwrite_implementation = true;
    }
    generate_bindings(&config)
}
