cmake_minimum_required(VERSION 3.1...3.15)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project("Cacher" VERSION 0.1.0.0 DESCRIPTION "Generic caching wrapper" LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)

include(CTest)
include(Catch)
enable_testing()

add_executable(cacher-test "tests/cacher.test.cpp")
target_include_directories(cacher-test PRIVATE "include")
catch_discover_tests(cacher-test)
