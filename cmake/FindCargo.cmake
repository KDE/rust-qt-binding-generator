# Find the cargo executable
#
# Defines the following variables
#  Cargo_FOUND      - True if the cargo executable was found
#  Cargo_EXECUTABLE - path of the cargo executable
#  Cargo_VERSION    - version number of cargo

#=============================================================================
# Copyright 2017 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

find_program(Cargo_EXECUTABLE NAMES cargo)

if (Cargo_EXECUTABLE)
    execute_process(COMMAND "${Cargo_EXECUTABLE}" --version
        OUTPUT_VARIABLE Cargo_VERSION_OUTPUT
        ERROR_VARIABLE Cargo_VERSION_ERROR
        RESULT_VARIABLE Cargo_VERSION_RESULT
    )
    if(NOT ${Cargo_VERSION_RESULT} EQUAL 0)
        message(SEND_ERROR "Command \"${Cargo_EXECUTABLE} --version\" failed with output:\n${Cargo_VERSION_ERROR}")
    else()
        # TODO: support also nightly
        string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
            Cargo_VERSION "${Cargo_VERSION_OUTPUT}"
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cargo
    REQUIRED_VARS Cargo_EXECUTABLE
    VERSION_VAR Cargo_VERSION
)

mark_as_advanced(Cargo_EXECUTABLE Cargo_VERSION)

set_package_properties(Cargo PROPERTIES
    DESCRIPTION "The Rust package manager"
    URL "https://github.com/rust-lang/cargo/"
)
