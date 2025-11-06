# Machine Summary Block
# {"file":"CMakeLists.txt","purpose":"Configures Glint3D targets and build switches.","exports":["glint","glint_core","user_paths_probe"],"depends_on":["engine/core","engine/modules","engine/platform","third_party"],"notes":["modular_migration_core_sources_relocated","third_party_vendored_layout","desktop_platform_sources","managed_dependency_fallbacks"]}
# Human Summary
# Root CMake script wiring core targets, third-party bridges, and migration-friendly include paths.

cmake_minimum_required(VERSION 3.15)
project(glint C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: KTX2/Basis (libktx)
option(ENABLE_KTX2 "Enable KTX2/Basis texture support via libktx" OFF)

# Modular scaffolding directories (transitional)
set(GLINT_ENGINE_CORE_DIR "engine/core")
set(GLINT_ENGINE_MODULES_DIR "engine/modules")
set(GLINT_ENGINE_PLATFORM_DIR "engine/platform")
set(GLINT_THIRD_PARTY_VENDORED_DIR "third_party/vendored")
set(GLINT_THIRD_PARTY_MANAGED_DIR "third_party/managed")
set(GLINT_IMGUI_VENDORED_DIR "${GLINT_THIRD_PARTY_VENDORED_DIR}/imgui")
set(GLINT_MINIZ_VENDORED_DIR "${GLINT_THIRD_PARTY_VENDORED_DIR}/miniz")
set(GLINT_TINYEXR_VENDORED_DIR "${GLINT_THIRD_PARTY_VENDORED_DIR}/tinyexr")

set(GLINT_ENGINE_PUBLIC_INCLUDE_DIRS
    engine/include
)

set(GLINT_ENGINE_PRIVATE_INCLUDE_DIRS
    ${GLINT_ENGINE_CORE_DIR}
    ${GLINT_ENGINE_CORE_DIR}/application
    ${GLINT_ENGINE_CORE_DIR}/scene
    ${GLINT_ENGINE_CORE_DIR}/rendering
    ${GLINT_ENGINE_CORE_DIR}/io
    ${GLINT_ENGINE_MODULES_DIR}
    ${GLINT_ENGINE_MODULES_DIR}/raytracing
    ${GLINT_ENGINE_MODULES_DIR}/gizmos
    ${GLINT_ENGINE_MODULES_DIR}/post_processing
    ${GLINT_ENGINE_PLATFORM_DIR}
    ${GLINT_PLATFORM_DESKTOP_DIR}
    ${GLINT_PLATFORM_DESKTOP_PANELS_DIR}
    ${GLINT_ENGINE_PLATFORM_DIR}/web
)

set(GLINT_THIRD_PARTY_INCLUDE_ROOTS
    ${GLINT_THIRD_PARTY_VENDORED_DIR}
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/stb
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/imgui
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/imgui/backends
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/glad
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/KHR
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/glm
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/rapidjson
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/OpenImageDenoise
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/tinyexr
    ${GLINT_THIRD_PARTY_VENDORED_DIR}/miniz
    ${GLINT_THIRD_PARTY_MANAGED_DIR}
    ${GLINT_THIRD_PARTY_MANAGED_DIR}/glfw/include
)

set(GLINT_CORE_PUBLIC_INCLUDE_DIRS ${GLINT_ENGINE_PUBLIC_INCLUDE_DIRS} ${GLINT_THIRD_PARTY_INCLUDE_ROOTS})
list(REMOVE_DUPLICATES GLINT_CORE_PUBLIC_INCLUDE_DIRS)

set(GLINT_CORE_PRIVATE_INCLUDE_DIRS ${GLINT_ENGINE_PRIVATE_INCLUDE_DIRS})
list(REMOVE_DUPLICATES GLINT_CORE_PRIVATE_INCLUDE_DIRS)

# Source lists
set(GLINT_PLATFORM_DESKTOP_DIR "${GLINT_ENGINE_PLATFORM_DIR}/desktop")
set(GLINT_PLATFORM_DESKTOP_PANELS_DIR "${GLINT_PLATFORM_DESKTOP_DIR}/panels")

set(GLINT_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources")
file(TO_CMAKE_PATH "${GLINT_RESOURCES_DIR}" GLINT_RESOURCES_DIR_NORMALIZED)

set(GLINT_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GLINT_GENERATED_DIR}")

set(GLINT_ICON_FILE "${GLINT_RESOURCES_DIR}/assets/icons/glint3d.ico")
file(TO_CMAKE_PATH "${GLINT_ICON_FILE}" GLINT_ICON_FILE_NORMALIZED)
set(GLINT_ICON_PATH "${GLINT_ICON_FILE_NORMALIZED}")

set(GLINT_RC_FILE "${GLINT_GENERATED_DIR}/glint3d.rc")
configure_file(resources/windows/glint3d.rc.in "${GLINT_RC_FILE}" @ONLY)

set(GLINT_CORE_APPLICATION_SOURCES
    ${GLINT_ENGINE_CORE_DIR}/application/application_core.cpp
    ${GLINT_ENGINE_CORE_DIR}/application/cli_parser.cpp
    ${GLINT_ENGINE_CORE_DIR}/application/render_settings.cpp
    ${GLINT_ENGINE_CORE_DIR}/application/schema_validator.cpp
)

set(GLINT_CORE_SCENE_SOURCES
    ${GLINT_ENGINE_CORE_DIR}/scene/scene_manager.cpp
    ${GLINT_ENGINE_CORE_DIR}/scene/light.cpp
    ${GLINT_ENGINE_CORE_DIR}/scene/material.cpp
    ${GLINT_ENGINE_CORE_DIR}/scene/json_ops.cpp
)

set(GLINT_CORE_RENDERING_SOURCES
    ${GLINT_ENGINE_CORE_DIR}/rendering/camera_controller.cpp
    ${GLINT_ENGINE_CORE_DIR}/rendering/render_system.cpp
    ${GLINT_ENGINE_CORE_DIR}/rendering/shader.cpp
    ${GLINT_ENGINE_CORE_DIR}/rendering/texture.cpp
    ${GLINT_ENGINE_CORE_DIR}/rendering/texture_cache.cpp
    ${GLINT_ENGINE_CORE_DIR}/rendering/skybox.cpp
    ${GLINT_ENGINE_CORE_DIR}/rendering/ibl_system.cpp
)

set(GLINT_CORE_IO_SOURCES
    ${GLINT_ENGINE_CORE_DIR}/io/objloader.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/mesh_loader.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/importer_registry.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/importers/obj_importer.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/importers/assimp_importer.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/assimp_loader.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/image_io.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/resource_paths.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/user_paths.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/path_security.cpp
)

option(GLINT_ENABLE_RAYTRACING "Enable the ray tracing module" ON)
option(GLINT_ENABLE_GIZMOS "Enable scene gizmo overlays" ON)

set(GLINT_MODULE_RAYTRACING_SOURCES
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/raytracer.cpp
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/BVHNode.cpp
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/triangle.cpp
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/RayUtils.cpp
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/brdf.cpp
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/microfacet_sampling.cpp
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/raytracer_lighting.cpp
    ${GLINT_ENGINE_MODULES_DIR}/raytracing/refraction.cpp
)

set(GLINT_MODULE_GIZMO_SOURCES
    ${GLINT_ENGINE_MODULES_DIR}/gizmos/gizmo.cpp
    ${GLINT_ENGINE_MODULES_DIR}/gizmos/grid.cpp
    ${GLINT_ENGINE_MODULES_DIR}/gizmos/axisrenderer.cpp
)

set(GLINT_PLATFORM_DESKTOP_SOURCES
    ${GLINT_PLATFORM_DESKTOP_DIR}/ui_bridge.cpp
    ${GLINT_PLATFORM_DESKTOP_DIR}/imgui_ui_layer.cpp
    ${GLINT_PLATFORM_DESKTOP_DIR}/file_dialog.cpp
    ${GLINT_PLATFORM_DESKTOP_PANELS_DIR}/scene_tree_panel.cpp
)

set(APP_SOURCES
    ${GLINT_ENGINE_CORE_DIR}/application/main.cpp
    ${GLINT_CORE_APPLICATION_SOURCES}
    ${GLINT_CORE_SCENE_SOURCES}
    ${GLINT_CORE_RENDERING_SOURCES}
    ${GLINT_CORE_IO_SOURCES}
    ${GLINT_PLATFORM_DESKTOP_SOURCES}
)

set(GLINT_CLI_SOURCES
    cli/src/config_resolver.cpp
    cli/src/init_scaffolder.cpp
    cli/src/init_command.cpp
    cli/src/ndjson_emitter.cpp
    cli/src/command_dispatcher.cpp
    cli/src/command_io.cpp
    cli/src/commands/validate_command.cpp
    cli/src/commands/config_command.cpp
    cli/src/commands/doctor_command.cpp
    cli/src/services/run_manifest_writer.cpp
    cli/src/services/workspace_locks.cpp
    cli/src/services/project_manifest.cpp
)

list(APPEND APP_SOURCES ${GLINT_CLI_SOURCES})

# Core library: expose engine without the app entry
set(CORE_SOURCES
    ${GLINT_CORE_APPLICATION_SOURCES}
    ${GLINT_CORE_SCENE_SOURCES}
    ${GLINT_CORE_RENDERING_SOURCES}
    ${GLINT_CORE_IO_SOURCES}
    ${GLINT_PLATFORM_DESKTOP_SOURCES}
)

set(GLINT_OPTIONAL_COMPILE_DEFINITIONS)

if(GLINT_ENABLE_RAYTRACING)
    list(APPEND APP_SOURCES ${GLINT_MODULE_RAYTRACING_SOURCES})
    list(APPEND CORE_SOURCES ${GLINT_MODULE_RAYTRACING_SOURCES})
    list(APPEND GLINT_OPTIONAL_COMPILE_DEFINITIONS GLINT_ENABLE_RAYTRACING=1)
else()
    list(APPEND GLINT_OPTIONAL_COMPILE_DEFINITIONS GLINT_ENABLE_RAYTRACING=0)
endif()

if(GLINT_ENABLE_GIZMOS)
    list(APPEND APP_SOURCES ${GLINT_MODULE_GIZMO_SOURCES})
    list(APPEND CORE_SOURCES ${GLINT_MODULE_GIZMO_SOURCES})
    list(APPEND GLINT_OPTIONAL_COMPILE_DEFINITIONS GLINT_ENABLE_GIZMOS=1)
else()
    list(APPEND GLINT_OPTIONAL_COMPILE_DEFINITIONS GLINT_ENABLE_GIZMOS=0)
endif()

set(IMGUI_SOURCES
    ${GLINT_IMGUI_VENDORED_DIR}/imgui.cpp
    ${GLINT_IMGUI_VENDORED_DIR}/imgui_draw.cpp
    ${GLINT_IMGUI_VENDORED_DIR}/imgui_tables.cpp
    ${GLINT_IMGUI_VENDORED_DIR}/imgui_widgets.cpp
    ${GLINT_IMGUI_VENDORED_DIR}/backends/imgui_impl_glfw.cpp
    ${GLINT_IMGUI_VENDORED_DIR}/backends/imgui_impl_opengl3.cpp
)

# Web (Emscripten) build is currently disabled; raise a clear error if selected.
if (EMSCRIPTEN)
    message(FATAL_ERROR "Web builds via Emscripten are temporarily disabled. See README.md for notes on re-enabling this path.")
endif()

# Desktop build
    add_library(glint_core STATIC ${CORE_SOURCES} ${IMGUI_SOURCES})
    target_include_directories(glint_core
        PUBLIC ${GLINT_CORE_PUBLIC_INCLUDE_DIRS}
        PRIVATE ${GLINT_CORE_PRIVATE_INCLUDE_DIRS}
    )
    target_compile_definitions(glint_core PUBLIC GLINT_RESOURCE_ROOT="${GLINT_RESOURCES_DIR_NORMALIZED}" ${GLINT_OPTIONAL_COMPILE_DEFINITIONS})

    add_executable(glint
        ${APP_SOURCES}
        ${IMGUI_SOURCES}
        ${GLINT_ENGINE_CORE_DIR}/rendering/glad.c
    )

    target_include_directories(glint PRIVATE ${GLINT_CORE_PRIVATE_INCLUDE_DIRS} cli/include)
    target_compile_definitions(glint PRIVATE GLINT_RESOURCE_ROOT="${GLINT_RESOURCES_DIR_NORMALIZED}" ${GLINT_OPTIONAL_COMPILE_DEFINITIONS})
    target_link_libraries(glint PRIVATE glint_core)

    # OpenGL
    find_package(OpenGL REQUIRED)

    # GLFW (prefer config, fallback to vcpkg/system hints + managed drop-in)
    find_package(glfw3 CONFIG QUIET)
    if (NOT glfw3_FOUND)
        find_package(glfw3 QUIET)
    endif()

    if (TARGET glfw OR TARGET glfw3)
        if (TARGET glfw)
            target_link_libraries(glint PRIVATE glfw OpenGL::GL)
        else()
            target_link_libraries(glint PRIVATE glfw3 OpenGL::GL)
        endif()
    else()
        message(WARNING "glfw3 not found via CMake packages. Trying manual locate (vcpkg/system hints).")
        # Best-effort manual lookup: vcpkg/system paths plus optional managed drop-in
        # Cover common Windows library name variants and locations.
        set(_GLFW_HINT_DIRS
            "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
            "$ENV{VCPKG_ROOT}/installed/x64-windows-static/lib"
            "$ENV{GLFW_ROOT}/lib"
            "$ENV{GLFW_HOME}/lib"
            "${CMAKE_SOURCE_DIR}/${GLINT_THIRD_PARTY_MANAGED_DIR}/glfw/lib"
        )
        find_library(GLFW3_LIB
            NAMES glfw3 glfw glfw3dll glfw3_mt glfw3_static
            HINTS ${_GLFW_HINT_DIRS}
            PATH_SUFFIXES "" lib lib64
        )
        # Try to find headers as well (for vendored headers or vcpkg)
        find_path(GLFW3_INCLUDE_DIR GLFW/glfw3.h
            HINTS
                "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
                "$ENV{VCPKG_ROOT}/installed/x64-windows-static/include"
                "$ENV{GLFW_ROOT}/include"
                "$ENV{GLFW_HOME}/include"
                "${CMAKE_SOURCE_DIR}/${GLINT_THIRD_PARTY_MANAGED_DIR}/glfw/include"
        )

        if (GLFW3_LIB)
            target_link_libraries(glint PRIVATE OpenGL::GL ${GLFW3_LIB})
            if (GLFW3_INCLUDE_DIR)
                target_include_directories(glint PRIVATE ${GLFW3_INCLUDE_DIR})
            endif()
        else()
            message(FATAL_ERROR "glfw3 not found. Install via vcpkg/system packages or place libs/headers under third_party/managed/glfw before configuring.")
        endif()
    endif()

    if (WIN32)
        target_link_libraries(glint PRIVATE gdi32 user32 shell32)
        
        # Add Windows resource file for application icon and version info
        if (MSVC)
            target_sources(glint PRIVATE "${GLINT_RC_FILE}")
            set_source_files_properties("${GLINT_RC_FILE}" PROPERTIES LANGUAGE RC)
        endif()
    endif()

    # ---- Assimp on desktop only ----
    find_package(assimp CONFIG QUIET)
    set(GLINT_ASSIMP_FOUND FALSE)
    if (assimp_FOUND)
        set(GLINT_ASSIMP_FOUND TRUE)
        target_link_libraries(glint PRIVATE assimp::assimp)
        target_link_libraries(glint_core PUBLIC assimp::assimp)
    else()
        set(_ASSIMP_MANAGED_ROOT "${CMAKE_SOURCE_DIR}/${GLINT_THIRD_PARTY_MANAGED_DIR}/assimp")
        find_library(GLINT_ASSIMP_MANUAL_LIB
            NAMES assimp assimp-vc143-mt assimp-vc142-mt
            PATHS "${_ASSIMP_MANAGED_ROOT}/lib"
            NO_DEFAULT_PATH
        )
        find_path(GLINT_ASSIMP_MANUAL_INCLUDE
            NAMES assimp/Importer.hpp
            PATHS "${_ASSIMP_MANAGED_ROOT}/include"
            NO_DEFAULT_PATH
        )
        if (GLINT_ASSIMP_MANUAL_LIB AND GLINT_ASSIMP_MANUAL_INCLUDE)
            set(GLINT_ASSIMP_FOUND TRUE)
            target_link_libraries(glint PRIVATE ${GLINT_ASSIMP_MANUAL_LIB})
            target_link_libraries(glint_core PUBLIC ${GLINT_ASSIMP_MANUAL_LIB})
            target_include_directories(glint PRIVATE ${GLINT_ASSIMP_MANUAL_INCLUDE})
            target_include_directories(glint_core PUBLIC ${GLINT_ASSIMP_MANUAL_INCLUDE})
            message(STATUS "Assimp found under ${_ASSIMP_MANAGED_ROOT}")
        endif()
    endif()
    if (GLINT_ASSIMP_FOUND)
        target_compile_definitions(glint PRIVATE HAVE_ASSIMP=1 USE_ASSIMP=1)
        target_compile_definitions(glint_core PRIVATE HAVE_ASSIMP=1 USE_ASSIMP=1)
    else()
        message(WARNING "Assimp not found. glTF/FBX/DAE will be disabled in this build.")
    endif()

    # Optional: Intel Open Image Denoise (desktop-only)
    find_package(OpenImageDenoise CONFIG QUIET)
    set(GLINT_OIDN_FOUND FALSE)
    if (OpenImageDenoise_FOUND)
        set(GLINT_OIDN_FOUND TRUE)
        target_link_libraries(glint PRIVATE OpenImageDenoise)
        target_link_libraries(glint_core PUBLIC OpenImageDenoise)
    else()
        set(_OIDN_MANAGED_ROOT "${CMAKE_SOURCE_DIR}/${GLINT_THIRD_PARTY_MANAGED_DIR}/openimagedenoise")
        find_library(GLINT_OIDN_MANUAL_LIB
            NAMES OpenImageDenoise
            PATHS "${_OIDN_MANAGED_ROOT}/lib"
            NO_DEFAULT_PATH
        )
        find_path(GLINT_OIDN_MANUAL_INCLUDE
            NAMES OpenImageDenoise/oidn.hpp
            PATHS "${_OIDN_MANAGED_ROOT}/include"
            NO_DEFAULT_PATH
        )
        if (GLINT_OIDN_MANUAL_LIB AND GLINT_OIDN_MANUAL_INCLUDE)
            set(GLINT_OIDN_FOUND TRUE)
            target_link_libraries(glint PRIVATE ${GLINT_OIDN_MANUAL_LIB})
            target_link_libraries(glint_core PUBLIC ${GLINT_OIDN_MANUAL_LIB})
            target_include_directories(glint PRIVATE ${GLINT_OIDN_MANUAL_INCLUDE})
            target_include_directories(glint_core PUBLIC ${GLINT_OIDN_MANUAL_INCLUDE})
            message(STATUS "OpenImageDenoise found under ${_OIDN_MANAGED_ROOT}")
        endif()
    endif()
    if (GLINT_OIDN_FOUND)
        target_compile_definitions(glint PRIVATE OIDN_ENABLED=1)
        target_compile_definitions(glint_core PRIVATE OIDN_ENABLED=1)
    else()
        message(STATUS "OpenImageDenoise not found; denoiser disabled.")
    endif()

    if (ENABLE_KTX2)
        find_package(ktx CONFIG QUIET)
        if (ktx_FOUND)
            if (TARGET KTX::ktx)
                target_link_libraries(glint PRIVATE KTX::ktx)
            elseif (TARGET ktx)
                target_link_libraries(glint PRIVATE ktx)
            else()
                message(WARNING "libktx found but target not recognized; link it manually.")
            endif()
            target_compile_definitions(glint PRIVATE KTX2_ENABLED=1)
        else()
            message(WARNING "ENABLE_KTX2=ON but libktx not found. KTX2 at runtime will be disabled.")
        endif()
    endif()

# Optional: EXR support via TinyEXR + miniz (header/libs must be vendored)
option(ENABLE_EXR "Enable OpenEXR loading via TinyEXR" ON)
if (ENABLE_EXR)
    set(TINYEXR_HEADER "${CMAKE_SOURCE_DIR}/${GLINT_TINYEXR_VENDORED_DIR}/tinyexr.h")
    set(MINIZ_HEADER   "${CMAKE_SOURCE_DIR}/${GLINT_MINIZ_VENDORED_DIR}/miniz.h")
    set(MINIZ_SOURCE   "${CMAKE_SOURCE_DIR}/${GLINT_MINIZ_VENDORED_DIR}/miniz.c")
    if (EXISTS ${TINYEXR_HEADER} AND EXISTS ${MINIZ_HEADER} AND EXISTS ${MINIZ_SOURCE})
        message(STATUS "TinyEXR/miniz found: enabling EXR support")
        target_compile_definitions(glint PRIVATE EXR_ENABLED=1)
        target_compile_definitions(glint_core PRIVATE EXR_ENABLED=1)
        # Add modular miniz sources if present
        set(_MINIZ_EXTRA_SRC)
        foreach(f miniz_tdef.c miniz_tinfl.c miniz_zip.c)
            if (EXISTS "${CMAKE_SOURCE_DIR}/${GLINT_MINIZ_VENDORED_DIR}/${f}")
                list(APPEND _MINIZ_EXTRA_SRC "${CMAKE_SOURCE_DIR}/${GLINT_MINIZ_VENDORED_DIR}/${f}")
            endif()
        endforeach()
        target_sources(glint PRIVATE ${MINIZ_SOURCE} ${_MINIZ_EXTRA_SRC})
        target_sources(glint_core PRIVATE ${MINIZ_SOURCE} ${_MINIZ_EXTRA_SRC})
    else()
        message(WARNING "TinyEXR/miniz not found under third_party/vendored; EXR support disabled. Place tinyexr.h, miniz.h, and miniz.c into third_party/vendored before configuring.")
    endif()
endif()

# Install rules
install(TARGETS glint RUNTIME DESTINATION bin)
install(TARGETS glint_core ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY engine/include/ DESTINATION include/glint)
install(FILES schemas/json_ops_v1.json DESTINATION share/glint/schemas)

add_executable(user_paths_probe
    tools/user_paths_probe.cpp
    ${GLINT_ENGINE_CORE_DIR}/io/user_paths.cpp
)
target_include_directories(user_paths_probe PRIVATE ${GLINT_CORE_PUBLIC_INCLUDE_DIRS} ${GLINT_CORE_PRIVATE_INCLUDE_DIRS})
target_compile_definitions(user_paths_probe PRIVATE ${GLINT_OPTIONAL_COMPILE_DEFINITIONS})
