cmake_minimum_required(VERSION 3.24)

project(RenderingPlayground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(RenderingPlayground RHIUsage.cpp)

target_include_directories(RenderingPlayground PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Hazel/src)

target_link_libraries(RenderingPlayground PRIVATE
        Hazel
        glm
        spdlog)