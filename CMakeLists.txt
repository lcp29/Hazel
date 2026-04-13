cmake_minimum_required(VERSION 3.24)

project(HazelWorkspace LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(HAZEL_BUILD_SCRIPTCORE "Build Hazel-ScriptCore" ON)
option(HAZEL_BUILD_HAZELNUT "Build Hazelnut" ON)

set(HAZEL_VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/vendor)

find_program(CLANG_FORMAT_EXE NAMES clang-format REQUIRED)

file(GLOB_RECURSE HAZEL_FORMAT_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Hazel/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Hazel/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Hazel/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Hazel/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Hazel/*.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Platform/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Platform/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Platform/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Platform/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/src/Platform/*.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazelnut/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazelnut/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazelnut/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazelnut/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Hazelnut/src/*.inl
)

add_custom_target(hazel-format
    COMMAND ${CLANG_FORMAT_EXE} --style=file -i ${HAZEL_FORMAT_FILES}
    COMMENT "Running clang-format on Hazel and Hazelnut sources"
)

add_subdirectory(${HAZEL_VENDOR_DIR}/Glad ${CMAKE_CURRENT_BINARY_DIR}/vendor/Glad)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${HAZEL_VENDOR_DIR}/GLFW ${CMAKE_CURRENT_BINARY_DIR}/vendor/GLFW)

add_subdirectory(${HAZEL_VENDOR_DIR}/imgui ${CMAKE_CURRENT_BINARY_DIR}/vendor/imgui)
add_subdirectory(${HAZEL_VENDOR_DIR}/ImGuizmo ${CMAKE_CURRENT_BINARY_DIR}/vendor/ImGuizmo)
add_subdirectory(${HAZEL_VENDOR_DIR}/stb_image ${CMAKE_CURRENT_BINARY_DIR}/vendor/stb_image)
add_subdirectory(${HAZEL_VENDOR_DIR}/entt ${CMAKE_CURRENT_BINARY_DIR}/vendor/entt)
add_subdirectory(${HAZEL_VENDOR_DIR}/filewatch ${CMAKE_CURRENT_BINARY_DIR}/vendor/filewatch)
add_subdirectory(${HAZEL_VENDOR_DIR}/glm ${CMAKE_CURRENT_BINARY_DIR}/vendor/glm)
add_subdirectory(${HAZEL_VENDOR_DIR}/yaml-cpp ${CMAKE_CURRENT_BINARY_DIR}/vendor/yaml-cpp)
add_subdirectory(${HAZEL_VENDOR_DIR}/spdlog ${CMAKE_CURRENT_BINARY_DIR}/vendor/spdlog)
add_subdirectory(${HAZEL_VENDOR_DIR}/vk-bootstrap ${CMAKE_CURRENT_BINARY_DIR}/vendor/vk-bootstrap)
add_subdirectory(${HAZEL_VENDOR_DIR}/tiny-obj-loader ${CMAKE_CURRENT_BINARY_DIR}/vendor/tiny-obj-loader)

add_subdirectory(Hazel)
add_subdirectory(Hazelnut/SandboxProject/Assets/Scripts)
add_subdirectory(Hazel-ScriptCore)
add_subdirectory(Hazelnut)

set_property(TARGET Hazel PROPERTY FOLDER "Core")
set_property(TARGET Hazel-ScriptCore PROPERTY FOLDER "Core")
set_property(TARGET Hazelnut PROPERTY FOLDER "Tools")

if(MSVC AND TARGET Hazelnut)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Hazelnut)
endif()
