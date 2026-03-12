project(filewatch LANGUAGES CXX)

add_library(filewatch INTERFACE)

target_include_directories(filewatch INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(filewatch INTERFACE cxx_std_17)
