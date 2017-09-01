include(FindPackageHandleStandardArgs)
find_program(Rust_EXECUTABLE rustc)
execute_process(COMMAND "${Rust_EXECUTABLE}" --version
    OUTPUT_VARIABLE Rust_VERSION_OUTPUT)
STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
    Rust_VERSION "${Rust_VERSION_OUTPUT}")
find_package_handle_standard_args(Rust
    REQUIRED_VARS Rust_EXECUTABLE
    VERSION_VAR Rust_VERSION)
mark_as_advanced(Rust_EXECUTABLE)
