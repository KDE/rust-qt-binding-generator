set(CMAKE_AUTORCC ON)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/testinterface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/testimplementation.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Bindings.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/tmp.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/rust_qt_binding_generator/rust_qt_binding_generator --overwrite-implementation "${CMAKE_CURRENT_SOURCE_DIR}/bindings.json"
  DEPENDS rust_qt_binding_generator bindings.json
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Tree.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/tmp.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/rust_qt_binding_generator/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/tree.json"
  DEPENDS rust_qt_binding_generator tree.json
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a"
  COMMAND cargo build ${RUST_BUILD_FLAG}
  DEPENDS rust/src/lib.rs
          rust/src/testimplementation.rs
          rust/src/testinterface.rs
          rust/src/implementation.rs
          rust/src/interface.rs
          rust/src/types.rs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust"
)
add_custom_target(rust_target DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a")

set(Demo_SRCS src/main.cpp src/Tree.cpp src/Bindings.cpp
    resource_file.qrc)

add_executable(Demo ${Demo_SRCS})
add_dependencies(Demo rust_target)

target_link_libraries(Demo
    Qt5::Quick
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::WidgetsAddons
    "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a"
)

set_target_properties(Demo PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
