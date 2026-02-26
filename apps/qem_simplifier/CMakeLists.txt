# Machine Summary Block
# {"file":"apps/qem_simplifier/CMakeLists.txt","purpose":"Builds the QEM vertical-slice scaffold targets (stub core, backend interfaces, CLI render bridge, shell UI).","exports":["glint_qem_core_stub","glint_qem_tool_backends","glint_qem_studio_shell"],"depends_on":["apps/qem_simplifier/include","apps/qem_simplifier/src"],"notes":["standalone_or_subdirectory","no_glint_link_dependency_yet"]}
# Human Summary
# CMake scaffold for a testable external-QEM tool slice without pulling in Glint viewer/render internals.

cmake_minimum_required(VERSION 3.15)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(glint_qem_tool_skeleton LANGUAGES CXX)
endif()

set(GLINT_QEM_TOOL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(GLINT_QEM_TOOL_INCLUDE_DIR ${GLINT_QEM_TOOL_ROOT}/include)

add_library(glint_qem_core_stub STATIC
    src/core/simplify_baseline.cpp
)
target_include_directories(glint_qem_core_stub PUBLIC ${GLINT_QEM_TOOL_INCLUDE_DIR})
target_compile_features(glint_qem_core_stub PUBLIC cxx_std_17)

add_library(glint_qem_tool_backends STATIC
    src/backends/qem_core_simplifier_backend.cpp
    src/backends/glint_cli_render_backend.cpp
    src/backends/mesh_preview_staging.cpp
    src/backends/obj_mesh_io_tinyobj.cpp
)
target_include_directories(glint_qem_tool_backends PUBLIC ${GLINT_QEM_TOOL_INCLUDE_DIR})
set(GLINT_QEM_TINYOBJ_HINT_DIR "${GLINT_QEM_TOOL_ROOT}/../../third_party/vendored/tinyobjloader")
if (EXISTS "${GLINT_QEM_TINYOBJ_HINT_DIR}")
    target_include_directories(glint_qem_tool_backends PUBLIC "${GLINT_QEM_TINYOBJ_HINT_DIR}")
endif()
target_compile_features(glint_qem_tool_backends PUBLIC cxx_std_17)
target_link_libraries(glint_qem_tool_backends PUBLIC glint_qem_core_stub)

add_executable(glint_qem_studio_shell
    src/ui/qem_studio_shell.cpp
)
target_include_directories(glint_qem_studio_shell PRIVATE ${GLINT_QEM_TOOL_INCLUDE_DIR})
target_compile_features(glint_qem_studio_shell PRIVATE cxx_std_17)
target_link_libraries(glint_qem_studio_shell PRIVATE glint_qem_tool_backends)

if (MSVC)
    target_compile_options(glint_qem_core_stub PRIVATE /W4)
    target_compile_options(glint_qem_tool_backends PRIVATE /W4)
    target_compile_options(glint_qem_studio_shell PRIVATE /W4)
else()
    target_compile_options(glint_qem_core_stub PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(glint_qem_tool_backends PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(glint_qem_studio_shell PRIVATE -Wall -Wextra -Wpedantic)
endif()
