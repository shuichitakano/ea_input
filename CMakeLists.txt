cmake_minimum_required(VERSION 3.13)
project(ea_input VERSION 0.1.0 LANGUAGES CXX)
add_library(ea_input STATIC
    src/hid_info.cpp src/pad_translator.cpp src/pad_state.cpp
    src/macro_profile.cpp src/switch_pro.cpp)
add_library(ea_input::ea_input ALIAS ea_input)
target_compile_features(ea_input PUBLIC cxx_std_17)
target_include_directories(ea_input PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ea_input>)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ea_input PRIVATE -Os -ffunction-sections -fdata-sections)
endif()
