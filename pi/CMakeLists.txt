cmake_minimum_required(VERSION 3.16)
project(vigil VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build flavour: simulated hardware on the laptop, real hardware on the Pi.
# Defaults follow the toolchain: cross-compiling selects the target build.
if(CMAKE_CROSSCOMPILING)
  set(_vigil_default_sim OFF)
else()
  set(_vigil_default_sim ON)
endif()

option(VIGIL_SIM         "Build with simulated hardware (laptop)"      ${_vigil_default_sim})
option(VIGIL_TARGET      "Build with real hardware (Raspberry Pi)"     ${CMAKE_CROSSCOMPILING})
option(VIGIL_BUILD_TESTS "Build the GoogleTest unit tests (host only)" OFF)

if((VIGIL_SIM AND VIGIL_TARGET) OR (NOT VIGIL_SIM AND NOT VIGIL_TARGET))
  message(FATAL_ERROR "Exactly one of VIGIL_SIM or VIGIL_TARGET must be ON")
endif()
if(VIGIL_TARGET AND VIGIL_BUILD_TESTS)
  message(FATAL_ERROR "Unit tests run on the laptop only; use VIGIL_SIM=ON")
endif()

find_package(Threads REQUIRED)

# Application library: everything except main(). Sources listed explicitly.
add_library(vigil_core STATIC
  src/logic/IntrusionFsm.cpp)
target_include_directories(vigil_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(vigil_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(vigil src/main.cpp)
target_compile_options(vigil PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(vigil PRIVATE
  VIGIL_SIM=$<BOOL:${VIGIL_SIM}>
  VIGIL_VERSION="${PROJECT_VERSION}")
target_link_libraries(vigil PRIVATE vigil_core Threads::Threads)
install(TARGETS vigil RUNTIME DESTINATION bin)

if(VIGIL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
