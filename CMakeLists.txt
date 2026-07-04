cmake_minimum_required(VERSION 3.15)
project(WeirdSys VERSION 1.0.0 LANGUAGES C CXX)

option(WEIRD_SYS_BUILD_TESTS "Build tests" OFF)
option(WEIRD_SYS_BUILD_DOCS "Build documentation" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(collector STATIC
    src/collector/registry_collector.cpp
    src/collector/wmi_collector.cpp
    src/collector/system_collector.cpp
)
target_include_directories(collector PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(diff_engine STATIC
    src/diff/diff_engine.cpp
)
target_include_directories(diff_engine PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(report_generator STATIC
    src/report/report_generator.cpp
)
target_include_directories(report_generator PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(weirdsys
    src/main.cpp
)
target_link_libraries(weirdsys PRIVATE
    collector
    diff_engine
    report_generator
)
target_include_directories(weirdsys PRIVATE ${CMAKE_SOURCE_DIR}/include)

install(TARGETS weirdsys collector diff_engine report_generator
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/weirdsys DESTINATION include)

if(WEIRD_SYS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
