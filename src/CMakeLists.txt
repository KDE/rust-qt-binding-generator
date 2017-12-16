
if (ECM_FOUND)
    ecm_create_qm_loader(rust_qt_binding_generator_QM_LOADER rust_qt_binding_generator_qt)
endif()

add_executable(rust_qt_binding_generator main.cpp
    parseJson.cpp cpp.cpp rust.cpp helper.cpp
    ${rust_qt_binding_generator_QM_LOADER}
)
target_link_libraries(rust_qt_binding_generator
    Qt5::Core
)
set_target_properties(rust_qt_binding_generator PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

if (ECM_FOUND)
    install(TARGETS rust_qt_binding_generator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
else()
    install(TARGETS rust_qt_binding_generator RUNTIME DESTINATION bin)
endif()
