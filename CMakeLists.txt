cmake_minimum_required(VERSION 3.20)
project(rasterizer CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_executable(rasterizer
src/main.cpp
src/texture.cpp
src/obj_loader.cpp
 "src/stb_image_impl.cpp")


# 头文件目录
target_include_directories(rasterizer PRIVATE ${CMAKE_SOURCE_DIR}/include)


# SDL2（使用 vcpkg/SDL2-config）
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(rasterizer PRIVATE SDL2::SDL2 SDL2::SDL2main)


# GLM（Header-only）
find_package(glm CONFIG REQUIRED)
target_link_libraries(rasterizer PRIVATE glm::glm)


# stb_image（仅需头文件）
find_path(STB_INCLUDE_DIR stb_image.h)
if (NOT STB_INCLUDE_DIR)
message(FATAL_ERROR "stb_image.h not found. Did you run: vcpkg install stb:x64-windows ?")
endif()
target_include_directories(rasterizer PRIVATE ${STB_INCLUDE_DIR})


if (MSVC)
target_compile_options(rasterizer PRIVATE /W4 /permissive-)
else()
target_compile_options(rasterizer PRIVATE -Wall -Wextra -Wpedantic)
endif()