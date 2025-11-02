cmake_minimum_required(VERSION 3.15)
project(glint C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: KTX2/Basis (libktx)
option(ENABLE_KTX2 "Enable KTX2/Basis texture support via libktx" OFF)

# Source lists
set(SRC_DIR engine/src)
set(IMGUI_DIR engine)

set(GLINT_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources")
file(TO_CMAKE_PATH "${GLINT_RESOURCES_DIR}" GLINT_RESOURCES_DIR_NORMALIZED)

set(GLINT_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GLINT_GENERATED_DIR}")

set(GLINT_ICON_FILE "${GLINT_RESOURCES_DIR}/assets/icons/glint3d.ico")
file(TO_CMAKE_PATH "${GLINT_ICON_FILE}" GLINT_ICON_FILE_NORMALIZED)
set(GLINT_ICON_PATH "${GLINT_ICON_FILE_NORMALIZED}")

set(GLINT_RC_FILE "${GLINT_GENERATED_DIR}/glint3d.rc")
configure_file(resources/windows/glint3d.rc.in "${GLINT_RC_FILE}" @ONLY)

set(APP_SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/application_core.cpp
    ${SRC_DIR}/scene_manager.cpp
    ${SRC_DIR}/render_system.cpp
    ${SRC_DIR}/camera_controller.cpp
    ${SRC_DIR}/ui_bridge.cpp
    ${SRC_DIR}/json_ops.cpp
    ${SRC_DIR}/cli_parser.cpp
    ${SRC_DIR}/render_settings.cpp
    ${SRC_DIR}/schema_validator.cpp
    ${SRC_DIR}/path_security.cpp
    ${SRC_DIR}/user_paths.cpp
    ${SRC_DIR}/imgui_ui_layer.cpp
    # ${SRC_DIR}/render_offscreen.cpp  # legacy (Application-based); excluded from build
    ${SRC_DIR}/shader.cpp
    ${SRC_DIR}/texture.cpp
    ${SRC_DIR}/texture_cache.cpp
    ${SRC_DIR}/grid.cpp
    ${SRC_DIR}/axisrenderer.cpp
    ${SRC_DIR}/gizmo.cpp
    ${SRC_DIR}/skybox.cpp
    ${SRC_DIR}/ibl_system.cpp
    ${SRC_DIR}/objloader.cpp
    ${SRC_DIR}/mesh_loader.cpp
    ${SRC_DIR}/importer_registry.cpp
    ${SRC_DIR}/importers/obj_importer.cpp
    ${SRC_DIR}/importers/assimp_importer.cpp
    ${SRC_DIR}/material.cpp
    ${SRC_DIR}/light.cpp
    ${SRC_DIR}/RayUtils.cpp
    ${SRC_DIR}/BVHNode.cpp
    ${SRC_DIR}/raytracer.cpp
    ${SRC_DIR}/assimp_loader.cpp
    ${SRC_DIR}/triangle.cpp
    ${SRC_DIR}/brdf.cpp
    ${SRC_DIR}/microfacet_sampling.cpp
    ${SRC_DIR}/raytracer_lighting.cpp
    ${SRC_DIR}/refraction.cpp
    ${SRC_DIR}/file_dialog.cpp
    ${SRC_DIR}/image_io.cpp
    ${SRC_DIR}/panels/scene_tree_panel.cpp
)

# Core library: expose engine without the app entry
set(CORE_SOURCES
    ${SRC_DIR}/application_core.cpp
    ${SRC_DIR}/scene_manager.cpp
    ${SRC_DIR}/render_system.cpp
    ${SRC_DIR}/camera_controller.cpp
    ${SRC_DIR}/ui_bridge.cpp
    ${SRC_DIR}/resource_paths.cpp
    ${SRC_DIR}/cli_parser.cpp
    ${SRC_DIR}/schema_validator.cpp
    ${SRC_DIR}/path_security.cpp
    ${SRC_DIR}/user_paths.cpp
    ${SRC_DIR}/imgui_ui_layer.cpp
    ${SRC_DIR}/shader.cpp
    ${SRC_DIR}/texture.cpp
    ${SRC_DIR}/texture_cache.cpp
    ${SRC_DIR}/grid.cpp
    ${SRC_DIR}/axisrenderer.cpp
    ${SRC_DIR}/gizmo.cpp
    ${SRC_DIR}/skybox.cpp
    ${SRC_DIR}/ibl_system.cpp
    ${SRC_DIR}/objloader.cpp
    ${SRC_DIR}/mesh_loader.cpp
    ${SRC_DIR}/importer_registry.cpp
    ${SRC_DIR}/importers/obj_importer.cpp
    ${SRC_DIR}/importers/assimp_importer.cpp
    ${SRC_DIR}/material.cpp
    ${SRC_DIR}/light.cpp
    ${SRC_DIR}/RayUtils.cpp
    ${SRC_DIR}/BVHNode.cpp
    ${SRC_DIR}/raytracer.cpp
    ${SRC_DIR}/assimp_loader.cpp
    ${SRC_DIR}/triangle.cpp
    ${SRC_DIR}/json_ops.cpp
    ${SRC_DIR}/brdf.cpp
    ${SRC_DIR}/microfacet_sampling.cpp
    ${SRC_DIR}/raytracer_lighting.cpp
    ${SRC_DIR}/refraction.cpp
    ${SRC_DIR}/file_dialog.cpp
    ${SRC_DIR}/image_io.cpp
    ${SRC_DIR}/panels/scene_tree_panel.cpp
)

set(IMGUI_SOURCES
    engine/Libraries/include/imgui/imgui.cpp
    engine/Libraries/include/imgui/imgui_draw.cpp
    engine/Libraries/include/imgui/imgui_tables.cpp
    engine/Libraries/include/imgui/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

# Web (Emscripten) build is currently disabled; raise a clear error if selected.
if (EMSCRIPTEN)
    message(FATAL_ERROR "Web builds via Emscripten are temporarily disabled. See README.md for notes on re-enabling this path.")
endif()

# Desktop build
    add_library(glint_core STATIC ${CORE_SOURCES} ${IMGUI_SOURCES})
    target_include_directories(glint_core PUBLIC
        engine/include
        engine
        engine/Libraries/include
        engine/Libraries/include/stb
        engine/Libraries/include/imgui
        engine/Libraries/include/imgui/backends)
    target_compile_definitions(glint_core PUBLIC GLINT_RESOURCE_ROOT="${GLINT_RESOURCES_DIR_NORMALIZED}")

    add_executable(glint
        ${APP_SOURCES}
        ${IMGUI_SOURCES}
        ${SRC_DIR}/glad.c
    )

    target_include_directories(glint PRIVATE
        engine/include
        engine
        engine/Libraries/include
        engine/Libraries/include/stb
        engine/Libraries/include/imgui
        engine/Libraries/include/imgui/backends
    )
    target_compile_definitions(glint PRIVATE GLINT_RESOURCE_ROOT="${GLINT_RESOURCES_DIR_NORMALIZED}")
    target_link_libraries(glint PRIVATE glint_core)

    # OpenGL
    find_package(OpenGL REQUIRED)

    # GLFW (prefer config, fallback to module, then vendored/vcpkg paths)
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
        message(WARNING "glfw3 not found via CMake packages. Trying manual locate (vcpkg and vendored lib).")
        # Best-effort manual lookup: vcpkg + vendored lib under engine/Libraries/lib
        # Cover common Windows library name variants and locations.
        set(_GLFW_HINT_DIRS
            "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
            "$ENV{VCPKG_ROOT}/installed/x64-windows-static/lib"
            "$ENV{GLFW_ROOT}/lib"
            "$ENV{GLFW_HOME}/lib"
            "${CMAKE_SOURCE_DIR}/engine/Libraries/lib"
            "${CMAKE_SOURCE_DIR}/engine/Libraries/lib/Release"
            "${CMAKE_SOURCE_DIR}/engine/Libraries/lib/Debug"
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
                "${CMAKE_SOURCE_DIR}/engine/Libraries/include"
        )

        if (GLFW3_LIB)
            target_link_libraries(glint PRIVATE OpenGL::GL ${GLFW3_LIB})
            if (GLFW3_INCLUDE_DIR)
                target_include_directories(glint PRIVATE ${GLFW3_INCLUDE_DIR})
            endif()
        else()
            message(FATAL_ERROR "glfw3 not found. Install via vcpkg and pass -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake or provide GLFW3 to CMake.")
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
    if (assimp_FOUND)
        target_link_libraries(glint PRIVATE assimp::assimp)
        target_compile_definitions(glint PRIVATE HAVE_ASSIMP=1 USE_ASSIMP=1)
    else()
        message(WARNING "Assimp not found. glTF/FBX/DAE will be disabled in this build.")
    endif()

    # Optional: Intel Open Image Denoise (desktop-only)
    find_package(OpenImageDenoise CONFIG QUIET)
    if (OpenImageDenoise_FOUND)
        target_link_libraries(glint PRIVATE OpenImageDenoise)
        target_compile_definitions(glint PRIVATE OIDN_ENABLED=1)
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
    set(TINYEXR_HEADER "${CMAKE_SOURCE_DIR}/engine/Libraries/include/tinyexr.h")
    set(MINIZ_HEADER   "${CMAKE_SOURCE_DIR}/engine/Libraries/include/miniz.h")
    set(MINIZ_SOURCE   "${CMAKE_SOURCE_DIR}/engine/Libraries/src/miniz.c")
    if (EXISTS ${TINYEXR_HEADER} AND EXISTS ${MINIZ_HEADER} AND EXISTS ${MINIZ_SOURCE})
        message(STATUS "TinyEXR/miniz found: enabling EXR support")
        target_compile_definitions(glint PRIVATE EXR_ENABLED=1)
        target_compile_definitions(glint_core PRIVATE EXR_ENABLED=1)
        # Add modular miniz sources if present
        set(_MINIZ_EXTRA_SRC)
        foreach(f miniz_tdef.c miniz_tinfl.c miniz_zip.c)
            if (EXISTS "${CMAKE_SOURCE_DIR}/engine/Libraries/src/${f}")
                list(APPEND _MINIZ_EXTRA_SRC "${CMAKE_SOURCE_DIR}/engine/Libraries/src/${f}")
            endif()
        endforeach()
        target_sources(glint PRIVATE ${MINIZ_SOURCE} ${_MINIZ_EXTRA_SRC})
        target_sources(glint_core PRIVATE ${MINIZ_SOURCE} ${_MINIZ_EXTRA_SRC})
        # Includes already contain engine/Libraries/include
    else()
        message(WARNING "TinyEXR/miniz not found under engine/Libraries; EXR support disabled. Place tinyexr.h, miniz.h in engine/Libraries/include and miniz.c in engine/Libraries/src")
    endif()
endif()

# Install rules
install(TARGETS glint RUNTIME DESTINATION bin)
install(TARGETS glint_core ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY engine/include/ DESTINATION include/glint)
install(FILES schemas/json_ops_v1.json DESTINATION share/glint/schemas)

add_executable(user_paths_probe
    tools/user_paths_probe.cpp
    engine/src/user_paths.cpp
)
target_include_directories(user_paths_probe PRIVATE engine/include)
