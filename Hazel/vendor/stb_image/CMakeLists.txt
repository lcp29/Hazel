project(stb_image LANGUAGES CXX)

add_library(stb_image STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h
)

target_include_directories(stb_image PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(stb_image PUBLIC cxx_std_17)

if(MSVC)
    set_property(TARGET stb_image PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()
