cmake_minimum_required(VERSION 3.24)

project(HazelWorkspace LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(HAZEL_BUILD_SCRIPTCORE "Build Hazel-ScriptCore" ON)
option(HAZEL_BUILD_HAZELNUT "Build Hazelnut" ON)
option(HAZEL_BUILD_SANDBOX "Build Sandbox" ON)

set(HAZEL_VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/vendor)

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

add_subdirectory(${HAZEL_VENDOR_DIR}/Box2D ${CMAKE_CURRENT_BINARY_DIR}/vendor/Box2D)

add_subdirectory(${HAZEL_VENDOR_DIR}/spdlog ${CMAKE_CURRENT_BINARY_DIR}/vendor/spdlog)

add_subdirectory(${HAZEL_VENDOR_DIR}/vk-bootstrap ${CMAKE_CURRENT_BINARY_DIR}/vendor/vk-bootstrap)

set(MSDF_ATLAS_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_DYNAMIC_RUNTIME ON CACHE BOOL "" FORCE)
add_subdirectory(${HAZEL_VENDOR_DIR}/msdf-atlas-gen ${CMAKE_CURRENT_BINARY_DIR}/vendor/msdf-atlas-gen)


add_subdirectory(Hazel)
add_subdirectory(Hazelnut/SandboxProject/Assets/Scripts)
add_subdirectory(Hazel-ScriptCore)
add_subdirectory(Hazelnut)
add_subdirectory(Sandbox)

set_property(TARGET Hazel PROPERTY FOLDER "Core")
set_property(TARGET Hazel-ScriptCore PROPERTY FOLDER "Core")
set_property(TARGET Hazelnut PROPERTY FOLDER "Tools")
set_property(TARGET Sandbox PROPERTY FOLDER "Misc")

if(MSVC AND TARGET Hazelnut)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Hazelnut)
endif()
