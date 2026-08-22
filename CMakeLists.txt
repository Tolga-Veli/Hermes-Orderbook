cmake_minimum_required(VERSION 3.20)

project(Hermes VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# neovim / clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

 
# Paths
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Options

option(HERMES_BUILD_TESTS "Build Hermes tests" ON)
option(HERMES_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviourSanitizer" OFF)

# Dependencies
add_subdirectory("${LIB_DIR}/FTXUI")

if(HERMES_BUILD_TESTS)
  enable_testing()
  add_subdirectory("${LIB_DIR}/googletest")
endif()

# Hermes

file(GLOB_RECURSE HERMES_SOURCES CONFIGURE_DEPENDS
  "${SRC_DIR}/*.inl"
  "${SRC_DIR}/*.cpp"
)

file(GLOB_RECURSE HERMES_HEADERS CONFIGURE_DEPENDS
  "${SRC_DIR}/*.hpp"
)

add_executable(Hermes ${HERMES_SOURCES} ${HERMES_HEADERS})

target_include_directories(Hermes PRIVATE "${SRC_DIR}")

target_link_libraries(Hermes PRIVATE ftxui::screen ftxui::dom ftxui::component)

source_group(TREE "${SRC_DIR}" PREFIX "src" FILES ${SOURCES} ${HEADERS})

# Sanitizers
if(HERMES_SANITIZERS)
  target_compile_options(Hermes PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(Hermes PRIVATE -fsanitize=address,undefined)
endif()

# Tests
if(HERMES_BUILD_TESTS)
  file(GLOB_RECURSE HERMES_TEST_SOURCES CONFIGURE_DEPENDS
      "${TEST_DIR}/*.cpp"
      "${TEST_DIR}/*.hpp"
  )

  add_executable(HermesTests ${HERMES_TEST_SOURCES})

  target_include_directories(HermesTests PRIVATE "${SRC_DIR}")

  target_link_libraries(HermesTests PRIVATE GTest::gtest_main)

  if(HERMES_SANITIZERS)
    target_compile_options(HermesTests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(HermesTests PRIVATE -fsanitize=address,undefined)
  endif()

  include(GoogleTest)
  gtest_discover_tests(HermesTests)
endif()

