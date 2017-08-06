
add_executable(rust_qt_binding_generator rust_qt_binding_generator.cpp)
target_link_libraries(rust_qt_binding_generator
    Qt5::Core
)
