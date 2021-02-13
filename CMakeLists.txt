cmake_minimum_required(VERSION 3.16)

project(tgs-viewer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(subprojects)

find_package(ZLIB REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(tgs-viewer)

target_sources(tgs-viewer PRIVATE main.cpp)
target_include_directories(tgs-viewer PRIVATE subprojects/rlottie/inc)
target_link_libraries(tgs-viewer PRIVATE glfw ZLIB::ZLIB rlottie::rlottie glad ${CMAKE_DL_LIBS})
