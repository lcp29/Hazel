project(Hazel LANGUAGES CXX)

set(HAZEL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(HAZEL_VENDOR_DIR ${HAZEL_ROOT}/vendor)

file(GLOB_RECURSE HAZEL_HEADERS CONFIGURE_DEPENDS
    ${HAZEL_ROOT}/src/*.h
)

file(GLOB_RECURSE HAZEL_SOURCES CONFIGURE_DEPENDS
    ${HAZEL_ROOT}/src/*.cpp
)

list(REMOVE_ITEM HAZEL_SOURCES
    ${HAZEL_ROOT}/src/Hazel/ImGui/ImGuiBuild.cpp
)

add_library(Hazel STATIC
    ${HAZEL_HEADERS}
    ${HAZEL_SOURCES}
)

target_compile_features(Hazel PUBLIC cxx_std_20)

target_precompile_headers(Hazel PRIVATE
    ${HAZEL_ROOT}/src/hzpch.h
)

target_compile_definitions(Hazel PUBLIC
    _CRT_SECURE_NO_WARNINGS
    GLFW_INCLUDE_NONE
    $<$<CONFIG:Debug>:HZ_DEBUG>
    $<$<CONFIG:Release>:HZ_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:HZ_RELEASE>
    $<$<CONFIG:MinSizeRel>:HZ_DIST>
)

target_include_directories(Hazel
    PUBLIC
        ${HAZEL_ROOT}/src
        ${HAZEL_VENDOR_DIR}/glm
        ${HAZEL_VENDOR_DIR}/spdlog/include
        ${HAZEL_VENDOR_DIR}/mono/include
)

set(VULKAN_SDK_DIR "$ENV{VULKAN_SDK}")
if(VULKAN_SDK_DIR)
    target_include_directories(Hazel PUBLIC
        ${VULKAN_SDK_DIR}/Include
    )
else()
    message(WARNING "VULKAN_SDK is not set. Hazel may fail to compile because shaderc/SPIRV-Cross headers and libraries are required.")
endif()

target_link_libraries(Hazel
    PUBLIC
        box2d
        glfw
        Glad
        ImGui
        ImGuizmo
        stb_image
        entt
        filewatch
        msdf-atlas-gen::msdf-atlas-gen
        yaml-cpp::yaml-cpp
        glm
        spdlog
        vk-bootstrap
)

if(MSVC)
    target_compile_options(Hazel PRIVATE /utf-8)
    set_property(TARGET Hazel PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

if(WIN32)
    target_link_libraries(Hazel PUBLIC
        opengl32
        ws2_32
        winmm
        version
        bcrypt
    )

    target_link_libraries(Hazel PUBLIC
        ${HAZEL_VENDOR_DIR}/mono/lib/$<IF:$<CONFIG:Debug>,Debug,Release>/libmono-static-sgen.lib
    )

    if(VULKAN_SDK_DIR)
        target_link_libraries(Hazel PUBLIC
            $<$<CONFIG:Debug>:${VULKAN_SDK_DIR}/Lib/shaderc_sharedd.lib>
            $<$<CONFIG:Debug>:${VULKAN_SDK_DIR}/Lib/spirv-cross-cored.lib>
            $<$<CONFIG:Debug>:${VULKAN_SDK_DIR}/Lib/spirv-cross-glsld.lib>
            $<$<NOT:$<CONFIG:Debug>>:${VULKAN_SDK_DIR}/Lib/shaderc_shared.lib>
            $<$<NOT:$<CONFIG:Debug>>:${VULKAN_SDK_DIR}/Lib/spirv-cross-core.lib>
            $<$<NOT:$<CONFIG:Debug>>:${VULKAN_SDK_DIR}/Lib/spirv-cross-glsl.lib>
        )
    endif()
endif()
