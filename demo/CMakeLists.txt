set(CMAKE_AUTORCC ON)

# generate c++ and rust code from fibonacci.json
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/fibonacci_interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/fibonacci_types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Fibonacci.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/Fibonacci.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/src/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/fibonacci.json"
  DEPENDS rust_qt_binding_generator fibonacci.json
)

# generate c++ and rust code from tree.json
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Tree.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/Tree.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/src/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/tree.json"
  DEPENDS rust_qt_binding_generator tree.json
)

# generate c++ and rust code from processes.json
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/processes_interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/processes_types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Processes.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/Processes.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/src/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/processes.json"
  DEPENDS rust_qt_binding_generator processes.json
)

# generate c++ and rust code from time_series.json
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/time_series_interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/time_series_types.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/TimeSeries.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/TimeSeries.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/src/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/time_series.json"
  DEPENDS rust_qt_binding_generator time_series.json
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
          rust/src/time_series_interface.rs
          rust/src/time_series_implementation.rs
          rust/src/time_series_types.rs
          rust/src/processes_interface.rs
          rust/src/processes_implementation.rs
          rust/src/processes_types.rs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust"
)
add_custom_target(rust_target DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a")

list(APPEND DemoLibs "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a")
list(APPEND DemoLibs Qt5::Widgets)
if (Qt5Quick_FOUND)
    list(APPEND DemoLibs Qt5::Quick)
endif()
if (Qt5QuickControls2_FOUND)
    add_definitions(-DQTQUICKCONTROLS2)
#    list(APPEND DemoLibs Qt5::QuickControls2)
endif()
if (Qt5Charts_FOUND)
    list(APPEND DemoLibs Qt5::Charts)
endif()

set(Demo_SRCS src/main.cpp src/Fibonacci.cpp src/Tree.cpp  src/TimeSeries.cpp
    src/modeltest.cpp src/Processes.cpp resource_file.qrc)

add_executable(Demo ${Demo_SRCS})
add_dependencies(Demo rust_target)

target_link_libraries(Demo ${DemoLibs})

set_target_properties(Demo PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
