project (rust_qt_binding_generator)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
cmake_policy(SET CMP0046 NEW)
cmake_policy(SET CMP0063 NEW)
set(QT_MIN_VERSION "5.6.0")
set(KF5_MIN_VERSION "5.2.0")

find_package(ECM 1.0.0 NO_MODULE)
if (ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
endif()

include(FeatureSummary)

find_package(Cargo)
find_package(Rust)

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
set(CMAKE_AUTOMOC ON)

if (ECM_FOUND)
    # Find KDE Modules
    find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
        CoreAddons      # KAboutData
        I18n            # KLocalizedString
        WidgetsAddons   # KMessageBox
    )
    find_package(KF5Kirigami CONFIG)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER STREQUAL DEBUG)
    set(RUST_TARGET_DIR target/debug/)
    set(RUST_BUILD_FLAG)
else()
    set(RUST_TARGET_DIR target/release/)
    set(RUST_BUILD_FLAG --release)
endif()

add_subdirectory(src)

enable_testing()

if(Cargo_FOUND AND Rust_FOUND)
    add_subdirectory(tests)
endif()


if(Qt5Widgets_FOUND AND Qt5Svg_FOUND AND Cargo_FOUND AND Rust_FOUND)
    add_subdirectory(demo)
endif()

