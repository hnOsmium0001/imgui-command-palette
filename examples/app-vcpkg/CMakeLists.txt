cmake_minimum_required(VERSION 3.12)
project(imgui-command-palette)

set(IMCMD_SRC_DIR "${CMAKE_SOURCE_DIR}/../.." CACHE STRING "The directory that contains imcmd_xxx.h|cpp files")
set(DEMO_SRC_DIR "${CMAKE_SOURCE_DIR}/../src" CACHE STRING "The directory that contains example app source files")

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_library(imcmd
    "${IMCMD_SRC_DIR}/imcmd_command_palette.h"
    "${IMCMD_SRC_DIR}/imcmd_command_palette.cpp"
    "${IMCMD_SRC_DIR}/imcmd_fuzzy_search.h"
    "${IMCMD_SRC_DIR}/imcmd_fuzzy_search.cpp"
)
target_compile_features(imcmd PUBLIC cxx_std_11)
target_link_libraries(imcmd PUBLIC imgui::imgui)

add_executable(imcmd-demo
    "${DEMO_SRC_DIR}/main.cpp"
)
target_compile_features(imcmd-demo PRIVATE cxx_std_11)
target_include_directories(imcmd-demo PRIVATE ${IMCMD_SRC_DIR})
target_link_libraries(imcmd-demo PRIVATE imcmd glfw imgui::imgui)
