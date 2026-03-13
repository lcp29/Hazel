project(ImGui LANGUAGES CXX)

include(CheckLanguage)

set(IMGUI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(IMGUI_CORE_SOURCES
    ${IMGUI_ROOT}/imgui.cpp
    ${IMGUI_ROOT}/imgui_demo.cpp
    ${IMGUI_ROOT}/imgui_draw.cpp
    ${IMGUI_ROOT}/imgui_tables.cpp
    ${IMGUI_ROOT}/imgui_widgets.cpp
    ${IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp
)

set(IMGUI_CORE_HEADERS
    ${IMGUI_ROOT}/imconfig.h
    ${IMGUI_ROOT}/imgui.h
    ${IMGUI_ROOT}/imgui_internal.h
    ${IMGUI_ROOT}/imstb_rectpack.h
    ${IMGUI_ROOT}/imstb_textedit.h
    ${IMGUI_ROOT}/imstb_truetype.h
)

set(IMGUI_BACKEND_SOURCES
    ${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp
    ${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_ROOT}/backends/imgui_impl_vulkan.cpp
)

set(IMGUI_BACKEND_HEADERS
    ${IMGUI_ROOT}/backends/imgui_impl_glfw.h
    ${IMGUI_ROOT}/backends/imgui_impl_opengl3.h
    ${IMGUI_ROOT}/backends/imgui_impl_opengl3_loader.h
    ${IMGUI_ROOT}/backends/imgui_impl_vulkan.h
    ${IMGUI_ROOT}/backends/imgui_impl_metal.h
)

if(APPLE)
    check_language(OBJCXX)
    if(CMAKE_OBJCXX_COMPILER)
        enable_language(OBJCXX)
        list(APPEND IMGUI_BACKEND_SOURCES
            ${IMGUI_ROOT}/backends/imgui_impl_metal.mm
        )
    else()
        message(WARNING "OBJCXX is unavailable, skipping ImGui Metal backend source.")
    endif()
endif()

add_library(ImGui STATIC
    ${IMGUI_CORE_SOURCES}
    ${IMGUI_CORE_HEADERS}
    ${IMGUI_BACKEND_SOURCES}
    ${IMGUI_BACKEND_HEADERS}
)

target_include_directories(ImGui PUBLIC
    ${IMGUI_ROOT}
    ${IMGUI_ROOT}/backends
)

target_compile_features(ImGui PUBLIC cxx_std_17)
target_compile_definitions(ImGui PUBLIC
    GLFW_INCLUDE_NONE
    IMGUI_DEFINE_MATH_OPERATORS
)

if(MSVC)
    set_property(TARGET ImGui PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

if(TARGET glfw)
    target_link_libraries(ImGui PUBLIC glfw)
elseif(TARGET glfw3::glfw)
    target_link_libraries(ImGui PUBLIC glfw3::glfw)
else()
    find_package(glfw3 CONFIG REQUIRED)
    if(TARGET glfw)
        target_link_libraries(ImGui PUBLIC glfw)
    elseif(TARGET glfw3::glfw)
        target_link_libraries(ImGui PUBLIC glfw3::glfw)
    else()
        message(FATAL_ERROR "GLFW target was not provided by glfw3.")
    endif()
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(ImGui PUBLIC Vulkan::Vulkan)

if(APPLE AND EXISTS "${IMGUI_ROOT}/backends/imgui_impl_metal.mm")
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(ImGui PUBLIC
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
    )
endif()
