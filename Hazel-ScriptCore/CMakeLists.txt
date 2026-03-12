project(Hazel-ScriptCore LANGUAGES NONE)

set(HAZEL_SCRIPTCORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(HAZEL_SCRIPTCORE_PROJECT ${HAZEL_SCRIPTCORE_ROOT}/Hazel-ScriptCore.csproj)
set(HAZEL_SCRIPTCORE_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Hazelnut/Resources/Scripts)
set(HAZEL_SCRIPTCORE_INTERMEDIATE_DIR ${HAZEL_SCRIPTCORE_OUTPUT_DIR}/Intermediates)

find_program(DOTNET_EXECUTABLE dotnet REQUIRED)

set(HAZEL_SCRIPTCORE_DOTNET_CONFIGURATION "$<IF:$<CONFIG:Debug>,Debug,Release>")
set(HAZEL_SCRIPTCORE_DLL ${HAZEL_SCRIPTCORE_OUTPUT_DIR}/Hazel-ScriptCore.dll)

file(GLOB_RECURSE HAZEL_SCRIPTCORE_SOURCES CONFIGURE_DEPENDS
    ${HAZEL_SCRIPTCORE_ROOT}/Source/*.cs
    ${HAZEL_SCRIPTCORE_ROOT}/Properties/*.cs
)

set(HAZEL_SCRIPTCORE_COMPILE_ITEMS "")
foreach(HAZEL_SCRIPTCORE_SOURCE ${HAZEL_SCRIPTCORE_SOURCES})
    file(TO_CMAKE_PATH "${HAZEL_SCRIPTCORE_SOURCE}" HAZEL_SCRIPTCORE_SOURCE_CMAKE_PATH)
    string(APPEND HAZEL_SCRIPTCORE_COMPILE_ITEMS "    <Compile Include=\"${HAZEL_SCRIPTCORE_SOURCE_CMAKE_PATH}\" />\n")
endforeach()

set(HAZEL_SCRIPTCORE_GENERATED_PROJECT ${CMAKE_CURRENT_BINARY_DIR}/Hazel-ScriptCore.csproj)

file(WRITE ${HAZEL_SCRIPTCORE_GENERATED_PROJECT}
"<Project Sdk=\"Microsoft.NET.Sdk\">
  <PropertyGroup>
    <TargetFramework>net472</TargetFramework>
    <AssemblyName>Hazel-ScriptCore</AssemblyName>
    <RootNamespace>Hazel</RootNamespace>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <GenerateAssemblyInfo>false</GenerateAssemblyInfo>
    <LangVersion>latest</LangVersion>
    <Nullable>disable</Nullable>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
  </PropertyGroup>
  <ItemGroup>
${HAZEL_SCRIPTCORE_COMPILE_ITEMS}  </ItemGroup>
</Project>
")

add_custom_command(
    OUTPUT ${HAZEL_SCRIPTCORE_DLL}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${HAZEL_SCRIPTCORE_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${HAZEL_SCRIPTCORE_INTERMEDIATE_DIR}
    COMMAND ${DOTNET_EXECUTABLE} build ${HAZEL_SCRIPTCORE_GENERATED_PROJECT}
        --configuration ${HAZEL_SCRIPTCORE_DOTNET_CONFIGURATION}
        /p:OutputPath=${HAZEL_SCRIPTCORE_OUTPUT_DIR}/
        /p:BaseIntermediateOutputPath=${HAZEL_SCRIPTCORE_INTERMEDIATE_DIR}/
        /p:IntermediateOutputPath=${HAZEL_SCRIPTCORE_INTERMEDIATE_DIR}/$<CONFIG>/
        /p:UseSharedCompilation=false
    DEPENDS
        ${HAZEL_SCRIPTCORE_PROJECT}
        ${HAZEL_SCRIPTCORE_GENERATED_PROJECT}
        ${HAZEL_SCRIPTCORE_SOURCES}
    WORKING_DIRECTORY ${HAZEL_SCRIPTCORE_ROOT}
    VERBATIM
)

add_custom_target(Hazel-ScriptCore ALL
    DEPENDS ${HAZEL_SCRIPTCORE_DLL}
)
