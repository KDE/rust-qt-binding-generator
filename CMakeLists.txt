project (RMail)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
cmake_policy(SET CMP0046 NEW)
cmake_policy(SET CMP0063 NEW)
set(QT_MIN_VERSION "5.3.0")
set(KF5_MIN_VERSION "5.2.0")

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(ExternalProject)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/Rust)

# Find Qt modules
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS 
    Core    # QCommandLineParser, QStringLiteral
    Quick
    Test
    Widgets # QApplication 
)

# Find KDE modules
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons      # KAboutData
    I18n            # KLocalizedString
    WidgetsAddons   # KMessageBox
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(rust_qt_binding_generator)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER STREQUAL DEBUG)
  set(RUST_TARGET_DIR target/debug/)
  set(RUST_BUILD_FLAG)
else()
  set(RUST_TARGET_DIR target/release/)
  set(RUST_BUILD_FLAG --release)
endif()

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/common-rust/src/testinterface.rs"
  COMMAND ${CMAKE_BINARY_DIR}/rust_qt_binding_generator/rust_qt_binding_generator "${CMAKE_CURRENT_SOURCE_DIR}/bindings.json"
  DEPENDS rust_qt_binding_generator bindings.json
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/common-rust/${RUST_TARGET_DIR}/librust.a"
  COMMAND cargo build ${RUST_BUILD_FLAG}
  DEPENDS common-rust/src/lib.rs
          common-rust/src/testimplementation.rs
          common-rust/src/testinterface.rs
          common-rust/src/implementation.rs
          common-rust/src/interface.rs
          common-rust/src/types.rs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/common-rust"
)
add_custom_target(rust_target DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/common-rust/${RUST_TARGET_DIR}/librust.a")

set(RMail_SRCS src/main.cpp src/RMailObject.cpp src/tmp.cpp)

add_executable(RMail ${RMail_SRCS})
add_dependencies(RMail rust_target)

target_link_libraries(RMail
    Qt5::Quick
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::WidgetsAddons
    "${CMAKE_CURRENT_SOURCE_DIR}/common-rust/${RUST_TARGET_DIR}/librust.a"
)

set_target_properties(RMail PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

install(TARGETS RMail ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)

