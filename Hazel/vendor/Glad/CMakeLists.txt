project(Glad LANGUAGES C)

add_library(Glad STATIC
    include/glad/glad.h
    include/KHR/khrplatform.h
    src/glad.c
)

target_include_directories(Glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    set_property(TARGET Glad PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()
