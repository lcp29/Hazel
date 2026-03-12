project(Hazelnut LANGUAGES CXX)

set(HAZELNUT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE HAZELNUT_HEADERS CONFIGURE_DEPENDS
    ${HAZELNUT_ROOT}/src/*.h
)

file(GLOB_RECURSE HAZELNUT_SOURCES CONFIGURE_DEPENDS
    ${HAZELNUT_ROOT}/src/*.cpp
)

if(NOT TARGET Hazel)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Hazel ${CMAKE_CURRENT_BINARY_DIR}/Hazel)
endif()

add_executable(Hazelnut
    ${HAZELNUT_HEADERS}
    ${HAZELNUT_SOURCES}
)

target_compile_features(Hazelnut PUBLIC cxx_std_17)

target_compile_definitions(Hazelnut PRIVATE
    $<$<CONFIG:Debug>:HZ_DEBUG>
    $<$<CONFIG:Release>:HZ_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:HZ_RELEASE>
    $<$<CONFIG:MinSizeRel>:HZ_DIST>
)

target_link_libraries(Hazelnut PRIVATE Hazel)

if(TARGET Hazel-ScriptCore)
    add_dependencies(Hazelnut Hazel-ScriptCore)
endif()

if(MSVC)
    target_compile_options(Hazelnut PRIVATE /utf-8)
    set_property(TARGET Hazelnut PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
    set_property(TARGET Hazelnut PROPERTY
        VS_DEBUGGER_WORKING_DIRECTORY ${HAZELNUT_ROOT}
    )
endif()
