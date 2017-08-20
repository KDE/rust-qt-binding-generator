set(CMAKE_AUTORCC ON)

# generate c++ and rust code from fibonacci.json
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/fibonacci_interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/fibonacci_types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Fibonacci.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/tmp.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/rust_qt_binding_generator/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/fibonacci.json"
  DEPENDS rust_qt_binding_generator fibonacci.json
)

# generate c++ and rust code from tree.json
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Tree.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/tmp.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/rust_qt_binding_generator/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/tree.json"
  DEPENDS rust_qt_binding_generator tree.json
)

# compile the rust code into a static library
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a"
  COMMAND cargo build ${RUST_BUILD_FLAG}
  DEPENDS rust/src/lib.rs
          rust/src/fibonacci_interface.rs
          rust/src/fibonacci_implementation.rs
          rust/src/fibonacci_types.rs
          rust/src/implementation.rs
          rust/src/interface.rs
          rust/src/types.rs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust"
)
add_custom_target(rust_target DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a")

if(Qt5Quick_FOUND)
    add_definitions(-DQTQUICK)
    set(Qt5Quick_LIBS Qt5::Quick)
endif()
if(Qt5QuickControls2_FOUND)
    add_definitions(-DQTQUICKCONTROLS2)
    set(Qt5QuickControls2_LIBS Qt5::QuickControls2)
endif()

set(Demo_SRCS src/main.cpp src/Tree.cpp src/Fibonacci.cpp
    resource_file.qrc)

add_executable(Demo ${Demo_SRCS})
add_dependencies(Demo rust_target)

target_link_libraries(Demo
    Qt5::Widgets
    "${Qt5Quick_LIBS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a"
)

set_target_properties(Demo PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
