include(FindPackageHandleStandardArgs)
find_program(Cargo_EXECUTABLE cargo)
execute_process(COMMAND "${Cargo_EXECUTABLE}" --version
    OUTPUT_VARIABLE Cargo_VERSION_OUTPUT)
STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
    Cargo_VERSION "${Cargo_VERSION_OUTPUT}")
find_package_handle_standard_args(Cargo
    REQUIRED_VARS Cargo_EXECUTABLE
    VERSION_VAR Cargo_VERSION)
mark_as_advanced(Cargo_EXECUTABLE)
