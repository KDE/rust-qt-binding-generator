enable_testing()

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust_object/src/interface.rs"
  COMMAND ${CMAKE_BINARY_DIR}/rust_qt_binding_generator/rust_qt_binding_generator --overwrite-implementation "${CMAKE_CURRENT_SOURCE_DIR}/test_object.json"
  DEPENDS rust_qt_binding_generator test_object.json
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust_object/${RUST_TARGET_DIR}/librust.a"
  COMMAND cargo build ${RUST_BUILD_FLAG}
  DEPENDS rust_object/src/lib.rs
          rust_object/src/implementation.rs
          rust_object/src/interface.rs
          rust_object/src/types.rs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust_object"
)
add_custom_target(test_rust_object DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rust_object/${RUST_TARGET_DIR}/librust.a")

add_executable(test_object test_object.cpp test_object_rust.cpp)
add_dependencies(test_object test_rust_object)
target_link_libraries(test_object
    Qt5::Core
    Qt5::Test
    "${CMAKE_CURRENT_SOURCE_DIR}/rust_object/${RUST_TARGET_DIR}/librust.a"
)

add_test(remove_test_object "${CMAKE_COMMAND}" -E remove test_object)
add_test(build_test_object "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target test_object)
set_tests_properties(build_test_object PROPERTIES DEPENDS remove_test_object)
add_test(test_object test_object)
set_tests_properties(test_object PROPERTIES DEPENDS build_test_object)
