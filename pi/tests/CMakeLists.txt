# GoogleTest: use an installed copy if present, otherwise fetch a pinned release.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)  # extracted files get the extraction time
  endif()
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(vigil_tests
  test_core_types.cpp
  test_intrusion_fsm.cpp)
target_compile_options(vigil_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(vigil_tests PRIVATE vigil_core GTest::gtest_main)
gtest_discover_tests(vigil_tests)
