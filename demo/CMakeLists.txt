set(CMAKE_AUTORCC ON)

# generate c++ and rust code from bindings.json
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/interface.rs"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/Bindings.h"
  # if the cpp file is marked GENERATED, CMake will not check it for moc
  #      "${CMAKE_CURRENT_SOURCE_DIR}/src/Bindings.cpp"
  COMMAND ${CMAKE_BINARY_DIR}/src/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/bindings.json"
  DEPENDS rust_qt_binding_generator bindings.json
)

# compile the rust code into a static library
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a"
  COMMAND ${Cargo_EXECUTABLE} build ${RUST_BUILD_FLAG}
  DEPENDS rust/src/lib.rs
          rust/src/implementation/mod.rs
          rust/src/implementation/time_series.rs
          rust/src/implementation/fibonacci.rs
          rust/src/implementation/processes.rs
          rust/src/interface.rs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust"
)
add_custom_target(rust_target DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a")

list(APPEND DemoLibs "${CMAKE_CURRENT_SOURCE_DIR}/rust/${RUST_TARGET_DIR}/librust.a")
list(APPEND DemoLibs Qt5::Widgets Qt5::Svg Threads::Threads ${DL_LIBRARY})
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

ecm_create_qm_loader(Demo_QM_LOADER Demo_qt.pot)

set(Demo_SRCS src/main.cpp src/Bindings.cpp src/SortedModel.cpp
    resource_file.qrc ${Demo_QM_LOADER})

add_executable(Demo ${Demo_SRCS})
add_dependencies(Demo rust_target)

target_link_libraries(Demo ${DemoLibs})

set_target_properties(Demo PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
