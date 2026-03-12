cmake_minimum_required(VERSION 3.24)

project(HazelWorkspace LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(HAZEL_BUILD_SCRIPTCORE "Build Hazel-ScriptCore" ON)
option(HAZEL_BUILD_HAZELNUT "Build Hazelnut" ON)
option(HAZEL_BUILD_SANDBOX "Build Sandbox" ON)

set(HAZEL_VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Hazel/vendor)

if(NOT TARGET Glad)
    add_subdirectory(${HAZEL_VENDOR_DIR}/Glad ${CMAKE_CURRENT_BINARY_DIR}/vendor/Glad)
endif()

if(NOT TARGET glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${HAZEL_VENDOR_DIR}/GLFW ${CMAKE_CURRENT_BINARY_DIR}/vendor/GLFW)
endif()

if(NOT TARGET ImGui)
    add_subdirectory(${HAZEL_VENDOR_DIR}/imgui ${CMAKE_CURRENT_BINARY_DIR}/vendor/imgui)
endif()

if(NOT TARGET ImGuizmo)
    add_subdirectory(${HAZEL_VENDOR_DIR}/ImGuizmo ${CMAKE_CURRENT_BINARY_DIR}/vendor/ImGuizmo)
endif()

if(NOT TARGET stb_image)
    add_subdirectory(${HAZEL_VENDOR_DIR}/stb_image ${CMAKE_CURRENT_BINARY_DIR}/vendor/stb_image)
endif()

if(NOT TARGET entt)
    add_subdirectory(${HAZEL_VENDOR_DIR}/entt ${CMAKE_CURRENT_BINARY_DIR}/vendor/entt)
endif()

if(NOT TARGET filewatch)
    add_subdirectory(${HAZEL_VENDOR_DIR}/filewatch ${CMAKE_CURRENT_BINARY_DIR}/vendor/filewatch)
endif()

if(NOT TARGET glm)
    add_subdirectory(${HAZEL_VENDOR_DIR}/glm ${CMAKE_CURRENT_BINARY_DIR}/vendor/glm)
endif()

if(NOT TARGET yaml-cpp)
    add_subdirectory(${HAZEL_VENDOR_DIR}/yaml-cpp ${CMAKE_CURRENT_BINARY_DIR}/vendor/yaml-cpp)
endif()

if(NOT TARGET box2d)
    add_subdirectory(${HAZEL_VENDOR_DIR}/Box2D ${CMAKE_CURRENT_BINARY_DIR}/vendor/Box2D)
endif()

if(NOT TARGET spdlog)
    add_subdirectory(${HAZEL_VENDOR_DIR}/spdlog ${CMAKE_CURRENT_BINARY_DIR}/vendor/spdlog)
endif()

if(NOT TARGET msdf-atlas-gen::msdf-atlas-gen)
    set(MSDF_ATLAS_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
    set(MSDF_ATLAS_USE_VCPKG OFF CACHE BOOL "" FORCE)
    set(MSDF_ATLAS_DYNAMIC_RUNTIME ON CACHE BOOL "" FORCE)
    add_subdirectory(${HAZEL_VENDOR_DIR}/msdf-atlas-gen ${CMAKE_CURRENT_BINARY_DIR}/vendor/msdf-atlas-gen)
endif()

add_subdirectory(Hazel)

add_subdirectory(Hazelnut/SandboxProject/Assets/Scripts)

if(HAZEL_BUILD_SCRIPTCORE)
    add_subdirectory(Hazel-ScriptCore)
endif()

if(HAZEL_BUILD_HAZELNUT)
    add_subdirectory(Hazelnut)
endif()

if(HAZEL_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif()

if(TARGET Hazel)
    set_property(TARGET Hazel PROPERTY FOLDER "Core")
endif()

if(TARGET Hazel-ScriptCore)
    set_property(TARGET Hazel-ScriptCore PROPERTY FOLDER "Core")
endif()

if(TARGET Hazelnut)
    set_property(TARGET Hazelnut PROPERTY FOLDER "Tools")
endif()

if(TARGET Sandbox)
    set_property(TARGET Sandbox PROPERTY FOLDER "Misc")
endif()

if(MSVC AND TARGET Hazelnut)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Hazelnut)
endif()
