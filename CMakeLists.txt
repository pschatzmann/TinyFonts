cmake_minimum_required(VERSION 3.16)
project(TinyFonts VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Header-only library: an INTERFACE target exposing src/ as include directory.
add_library(TinyFonts INTERFACE)
add_library(TinyFonts::TinyFonts ALIAS TinyFonts)
target_include_directories(TinyFonts INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(TinyFonts INTERFACE cxx_std_11)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(_tinyfonts_tests_default ON)
else()
  set(_tinyfonts_tests_default OFF)
endif()
option(TINYFONTS_BUILD_TESTS "Build the host unit tests" ${_tinyfonts_tests_default})

if(TINYFONTS_BUILD_TESTS)
  enable_testing()
  add_executable(tinyfonts_tests test/test_tinyfonts.cpp)
  target_link_libraries(tinyfonts_tests PRIVATE TinyFonts::TinyFonts)
  target_compile_options(tinyfonts_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME tinyfonts_tests COMMAND tinyfonts_tests)
endif()

# Desktop examples. The TinyGPU example needs TinyGPU and the Arduino
# runtime (Arduino-Emulator), which are fetched here unless the sources are
# supplied: -DTINYFONTS_TINYGPU_DIR=<path to TinyGPU checkout>, and
# -DFETCHCONTENT_SOURCE_DIR_ARDUINO_EMULATOR=<path to Arduino-Emulator>.
option(TINYFONTS_BUILD_EXAMPLES "Build the desktop examples" OFF)
if(TINYFONTS_BUILD_EXAMPLES)
  set(TINYFONTS_TINYGPU_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../TinyGPU" CACHE PATH
      "Path to the TinyGPU library sources")

  include(FetchContent)
  FetchContent_Declare(
    arduino_emulator
    GIT_REPOSITORY https://github.com/pschatzmann/Arduino-Emulator.git
    GIT_TAG        main
    CMAKE_ARGS -DUSE_RPI=OFF
  )
  FetchContent_MakeAvailable(arduino_emulator)

  if(NOT TARGET TinyGPU)
    set(TGPU_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${TINYFONTS_TINYGPU_DIR} ${CMAKE_BINARY_DIR}/TinyGPU)
  endif()

  add_subdirectory(examples/tinygpu)
endif()

# Installation and package config, so other CMake projects can use
#   find_package(TinyFonts) and target_link_libraries(... TinyFonts::TinyFonts)
install(TARGETS TinyFonts EXPORT TinyFontsTargets)
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(EXPORT TinyFontsTargets
        FILE TinyFontsTargets.cmake
        NAMESPACE TinyFonts::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyFonts)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TinyFontsConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/TinyFontsConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyFonts)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/TinyFontsConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/TinyFontsConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/TinyFontsConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyFonts)
