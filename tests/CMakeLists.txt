
set(GENERATOR "${RustQtBindingGenerator_EXECUTABLE}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_custom_target("clean-rust")

function(rust_test NAME DIRECTORY)
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}")
    set(DIR "${SRC}/${DIRECTORY}")
    if (MSVC)
      SET(RUST_LIB "${DIR}/${RUST_TARGET_DIR}/rust.lib")
    else()
      SET(RUST_LIB "${DIR}/${RUST_TARGET_DIR}/librust.a")
    endif()

    set_property(SOURCE "${SRC}/${NAME}_rust.h" PROPERTY SKIP_AUTOGEN OFF)
    add_custom_command(
        OUTPUT "${DIR}/src/interface.rs"
               "${SRC}/${NAME}_rust.h"
               "${SRC}/${NAME}_rust.cpp"
        COMMAND "${GENERATOR}" "${SRC}/${NAME}.json"
        MAIN_DEPENDENCY "${NAME}.json"
        DEPENDS rust_qt_binding_generator
    )
    add_custom_command(
        OUTPUT "${RUST_LIB}"
        COMMAND ${Cargo_EXECUTABLE} build ${RUST_BUILD_FLAG}
        DEPENDS "${DIR}/src/lib.rs"
                "${DIR}/src/implementation.rs"
                "${DIR}/src/interface.rs"
        WORKING_DIRECTORY "${DIR}"
    )
    add_custom_target("test_${DIRECTORY}"
        DEPENDS "${RUST_LIB}")

    add_executable("${NAME}" "${NAME}.cpp" "${NAME}_rust.cpp" "${NAME}_rust.h")
    set_target_properties("${NAME}" PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
    )
    add_dependencies("${NAME}" "test_${DIRECTORY}")
    target_link_libraries("${NAME}"
        Qt5::Core
        Qt5::Test
        "${RUST_LIB}"
        Threads::Threads ${DL_LIBRARY}
    )

    add_test("build_${NAME}"
            "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target "${NAME}")
    add_test("${NAME}" "${NAME}")
    set_tests_properties("${NAME}" PROPERTIES DEPENDS "build_${NAME}")

    add_custom_command(
        OUTPUT "clean_${NAME}"
        COMMAND ${Cargo_EXECUTABLE} clean
        WORKING_DIRECTORY "${DIR}"
    )
    add_custom_target("clean-${NAME}" DEPENDS "clean_${NAME}")
    add_dependencies("clean-rust" "clean-${NAME}")


endfunction(rust_test)

rust_test(test_object rust_object)
rust_test(test_object_types rust_object_types)
rust_test(test_list rust_list)
rust_test(test_list_types rust_list_types)
rust_test(test_tree rust_tree)
rust_test(test_objects rust_objects)
rust_test(test_functions rust_functions)
