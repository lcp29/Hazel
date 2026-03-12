project(Sandbox LANGUAGES CXX)

set(SANDBOX_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SANDBOX_HEADERS CONFIGURE_DEPENDS
    ${SANDBOX_ROOT}/src/*.h
)

file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS
    ${SANDBOX_ROOT}/src/*.cpp
)

if(NOT TARGET Hazel)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Hazel ${CMAKE_CURRENT_BINARY_DIR}/Hazel)
endif()

add_executable(Sandbox
    ${SANDBOX_HEADERS}
    ${SANDBOX_SOURCES}
)

target_compile_features(Sandbox PUBLIC cxx_std_17)

target_compile_definitions(Sandbox PRIVATE
    $<$<CONFIG:Debug>:HZ_DEBUG>
    $<$<CONFIG:Release>:HZ_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:HZ_RELEASE>
    $<$<CONFIG:MinSizeRel>:HZ_DIST>
)

target_link_libraries(Sandbox PRIVATE Hazel)

if(MSVC)
    target_compile_options(Sandbox PRIVATE /utf-8)
    set_property(TARGET Sandbox PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
    set_property(TARGET Sandbox PROPERTY
        VS_DEBUGGER_WORKING_DIRECTORY ${SANDBOX_ROOT}
    )
endif()
