cmake_minimum_required(VERSION 3.24)

project(RenderingPlayground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(RHITest RHITest.cpp)

target_include_directories(RHITest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Hazel/src)

target_link_libraries(RHITest PRIVATE
        Hazel
        glm
        spdlog)

add_executable(RHIRendering RHIRendering.cpp)
target_link_libraries(RHIRendering PRIVATE Hazel)