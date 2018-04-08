cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(ProtocolSession CXX)

option(build_tests "build tests" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 11)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories("${CMAKE_SOURCE_DIR}/include")

set(
  library_sources
    src/PieceInformation.cpp
    src/common.cpp
    src/PieceDeliveryPipeline.cpp
    src/SpeedTestPolicy.cpp
)

# === build library ===
add_library(protocol_session ${library_sources})

# === build tests ===
if(build_tests)
  set(
    test_common_sources
      test/SessionTest.cpp
  )

  # === test library ===
  include_directories("${CMAKE_SOURCE_DIR}/test")

  add_library(test_common ${test_common_sources})

  file(GLOB tests RELATIVE "${CMAKE_SOURCE_DIR}" "test/test_*.cpp")

  enable_testing()

  foreach(s ${tests})
    get_filename_component (sn ${s} NAME_WE)
    add_executable(${sn} ${s})
    target_link_libraries(${sn} test_common protocol_session ${CONAN_LIBS})
    add_test(${sn} "bin/${sn}")
  endforeach(s)

endif()
