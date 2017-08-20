project (rust_qt_binding_generator)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
cmake_policy(SET CMP0046 NEW)
cmake_policy(SET CMP0063 NEW)
set(QT_MIN_VERSION "5.6.0")

include(FeatureSummary)

# Find Qt modules
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS 
    Core
    Test
)
find_package(Qt5Widgets ${QT_MIN_VERSION} CONFIG)
find_package(Qt5Quick ${QT_MIN_VERSION} CONFIG)
set(CMAKE_AUTOMOC ON)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER STREQUAL DEBUG)
  set(RUST_TARGET_DIR target/debug/)
  set(RUST_BUILD_FLAG)
else()
  set(RUST_TARGET_DIR target/release/)
  set(RUST_BUILD_FLAG --release)
endif()

add_subdirectory(rust_qt_binding_generator)
enable_testing()
add_subdirectory(tests)

if(Qt5Widgets_FOUND)
  add_subdirectory(demo)
endif()

