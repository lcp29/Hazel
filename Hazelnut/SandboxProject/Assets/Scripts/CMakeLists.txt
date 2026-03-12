cmake_minimum_required(VERSION 3.24)

project(SandboxScripts LANGUAGES NONE)

set(SANDBOX_SCRIPTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SANDBOX_PROJECT_NAME Sandbox)
set(SANDBOX_SCRIPTS_BINARY_DIR ${SANDBOX_SCRIPTS_ROOT}/Binaries)
set(SANDBOX_SCRIPTS_INTERMEDIATE_DIR ${SANDBOX_SCRIPTS_ROOT}/Intermediates)
set(SANDBOX_SCRIPTS_PROJECT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SANDBOX_PROJECT_NAME}.csproj)
set(SANDBOX_SCRIPTS_DLL ${SANDBOX_SCRIPTS_BINARY_DIR}/${SANDBOX_PROJECT_NAME}.dll)
set(HAZEL_SCRIPTCORE_PROJECT ${SANDBOX_SCRIPTS_ROOT}/../../../../Hazel-ScriptCore/Hazel-ScriptCore.csproj)

find_program(DOTNET_EXECUTABLE dotnet REQUIRED)

set(SANDBOX_SCRIPTS_DOTNET_CONFIGURATION "$<IF:$<CONFIG:Debug>,Debug,Release>")

file(GLOB_RECURSE SANDBOX_SCRIPTS_SOURCE_FILES CONFIGURE_DEPENDS
    ${SANDBOX_SCRIPTS_ROOT}/Source/*.cs
    ${SANDBOX_SCRIPTS_ROOT}/Properties/*.cs
)

file(TO_CMAKE_PATH "${HAZEL_SCRIPTCORE_PROJECT}" HAZEL_SCRIPTCORE_PROJECT_CMAKE_PATH)

set(SANDBOX_SCRIPTS_COMPILE_ITEMS "")
foreach(SANDBOX_SCRIPT_SOURCE ${SANDBOX_SCRIPTS_SOURCE_FILES})
    file(TO_CMAKE_PATH "${SANDBOX_SCRIPT_SOURCE}" SANDBOX_SCRIPT_SOURCE_CMAKE_PATH)
    string(APPEND SANDBOX_SCRIPTS_COMPILE_ITEMS "    <Compile Include=\"${SANDBOX_SCRIPT_SOURCE_CMAKE_PATH}\" />\n")
endforeach()

file(WRITE ${SANDBOX_SCRIPTS_PROJECT_FILE}
"<Project Sdk=\"Microsoft.NET.Sdk\">
  <PropertyGroup>
    <TargetFramework>net472</TargetFramework>
    <AssemblyName>${SANDBOX_PROJECT_NAME}</AssemblyName>
    <RootNamespace>${SANDBOX_PROJECT_NAME}</RootNamespace>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <GenerateAssemblyInfo>false</GenerateAssemblyInfo>
    <LangVersion>latest</LangVersion>
    <Nullable>disable</Nullable>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
  </PropertyGroup>
  <ItemGroup>
    <ProjectReference Include=\"${HAZEL_SCRIPTCORE_PROJECT_CMAKE_PATH}\" />
  </ItemGroup>
  <ItemGroup>
${SANDBOX_SCRIPTS_COMPILE_ITEMS}  </ItemGroup>
</Project>
")

add_custom_command(
    OUTPUT ${SANDBOX_SCRIPTS_DLL}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SANDBOX_SCRIPTS_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SANDBOX_SCRIPTS_INTERMEDIATE_DIR}
    COMMAND ${DOTNET_EXECUTABLE} build ${SANDBOX_SCRIPTS_PROJECT_FILE}
        --configuration ${SANDBOX_SCRIPTS_DOTNET_CONFIGURATION}
        /p:OutputPath=${SANDBOX_SCRIPTS_BINARY_DIR}/
        /p:BaseIntermediateOutputPath=${SANDBOX_SCRIPTS_INTERMEDIATE_DIR}/
        /p:IntermediateOutputPath=${SANDBOX_SCRIPTS_INTERMEDIATE_DIR}/$<CONFIG>/
        /p:UseSharedCompilation=false
    DEPENDS
        ${SANDBOX_SCRIPTS_PROJECT_FILE}
        ${HAZEL_SCRIPTCORE_PROJECT}
        ${SANDBOX_SCRIPTS_SOURCE_FILES}
    WORKING_DIRECTORY ${SANDBOX_SCRIPTS_ROOT}
    VERBATIM
)

add_custom_target(SandboxScripts ALL
    DEPENDS ${SANDBOX_SCRIPTS_DLL}
)
