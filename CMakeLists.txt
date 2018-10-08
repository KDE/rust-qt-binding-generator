cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project (rust_qt_binding_generator)

set(QT_MIN_VERSION "5.6.0")
set(KF5_MIN_VERSION "5.2.0")

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(ECM 1.0.0 NO_MODULE)
if (ECM_FOUND)
    LIST(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings NO_POLICY_SCOPE)
    include(ECMPoQmTools)
else()
    cmake_policy(SET CMP0063 NEW)
    cmake_policy(SET CMP0071 NEW)
    enable_testing()
    set(CMAKE_AUTOMOC ON)
endif()

set(CMAKE_AUTOUIC ON)

include(FeatureSummary)

find_package(Cargo REQUIRED)
find_package(Rust REQUIRED)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

# Runtime information library -- dl on Debian, execinfo on FreeBSD.
# This quiets a warning from rustc about linking to native artifacts.
if (CMAKE_SYSTEM MATCHES "FreeBSD")
    set(NEED_DL_LIBRARY true)
    find_library(DL_LIBRARY execinfo)
elseif (DEFINED CMAKE_DL_LIBS)
    set(NEED_DL_LIBRARY true)
    find_library(DL_LIBRARY ${CMAKE_DL_LIBS})
endif()

# It would be neater to use set_package_properties() here and to
# rely on feature_summary(), below, but that entails using find_package()
# and moving this whole thing into a Find-module, which is over-wrought.
if (NEED_DL_LIBRARY AND NOT DL_LIBRARY)
    message(FATAL_ERROR "No runtime information library (-ldl or -lexecinfo)")
endif()

# Find Qt modules
find_package(Qt5 ${QT_MIN_VERSION} CONFIG
    REQUIRED COMPONENTS
        Core
        Test
    OPTIONAL_COMPONENTS
        Widgets
        Svg
        Quick
)
find_package(Qt5QuickControls2 EXACT ${Qt5Core_VERSION})
find_package(Qt5Charts EXACT ${Qt5Core_VERSION})

find_package(KF5Kirigami2)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER STREQUAL DEBUG)
    set(RUST_TARGET_DIR target/debug/)
    set(RUST_BUILD_FLAG)
else()
    set(RUST_TARGET_DIR target/release/)
    set(RUST_BUILD_FLAG --release)
endif()

set(RustQtBindingGenerator_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/${RUST_TARGET_DIR}rust_qt_binding_generator")
add_custom_command(
  OUTPUT "${RustQtBindingGenerator_EXECUTABLE}"
  COMMAND ${Cargo_EXECUTABLE} build ${RUST_BUILD_FLAG}
  DEPENDS src/lib.rs
          src/configuration.rs
          src/cpp.rs
          src/rust.rs
          src/util.rs
          src/bin/rust_qt_binding_generator.rs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
add_custom_target(rust_qt_binding_generator DEPENDS "${RustQtBindingGenerator_EXECUTABLE}")

add_subdirectory(tests)

if(Qt5Widgets_FOUND AND Qt5Svg_FOUND)
    add_subdirectory(demo)
endif()

if (ECM_FOUND)
    install(FILES org.kde.rust_qt_binding_generator.appdata.xml
            DESTINATION ${KDE_INSTALL_METAINFODIR})
endif()
