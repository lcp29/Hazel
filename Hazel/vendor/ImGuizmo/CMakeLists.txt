project(ImGuizmo LANGUAGES CXX)

set(IMGUIZMO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(IMGUIZMO_SOURCES
    ${IMGUIZMO_ROOT}/ImGuizmo.cpp
    ${IMGUIZMO_ROOT}/GraphEditor.cpp
    ${IMGUIZMO_ROOT}/ImCurveEdit.cpp
    ${IMGUIZMO_ROOT}/ImGradient.cpp
    ${IMGUIZMO_ROOT}/ImSequencer.cpp
)

set(IMGUIZMO_HEADERS
    ${IMGUIZMO_ROOT}/ImGuizmo.h
    ${IMGUIZMO_ROOT}/GraphEditor.h
    ${IMGUIZMO_ROOT}/ImCurveEdit.h
    ${IMGUIZMO_ROOT}/ImGradient.h
    ${IMGUIZMO_ROOT}/ImSequencer.h
    ${IMGUIZMO_ROOT}/ImZoomSlider.h
)

add_library(ImGuizmo STATIC
    ${IMGUIZMO_SOURCES}
    ${IMGUIZMO_HEADERS}
)

target_include_directories(ImGuizmo PUBLIC
    ${IMGUIZMO_ROOT}
)

target_compile_features(ImGuizmo PUBLIC cxx_std_11)
target_compile_definitions(ImGuizmo PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

if(TARGET ImGui)
    target_link_libraries(ImGuizmo PUBLIC ImGui)
else()
    target_include_directories(ImGuizmo PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/../imgui/backends
    )
endif()

if(MSVC)
    set_property(TARGET ImGuizmo PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()
