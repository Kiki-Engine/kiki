cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW)

project(KikiEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Ensures deps build as .lib, not .dll

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MSDFGEN_USE_DYNAMIC_CRT ON CACHE BOOL "" FORCE)      
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_DYNAMIC_RUNTIME ON CACHE BOOL "" FORCE)

set(FREETYPE_LIBRARY_TYPE STATIC CACHE INTERNAL "")

option(ENABLE_TRACY "Enable Tracy for profiling" ON)

if (APPLE)
option(ENABLE_TRACY_VK "Enable Tracy for Vulkan GPU profiling" OFF)
else()
option(ENABLE_TRACY_VK "Enable Tracy for Vulkan GPU profiling" ON)
endif()


# --- Jolt Physics Configuration ---
# Must be set before FetchContent to ensure Jolt builds with correct settings
# Use dynamic MSVC runtime to match the rest of the engine (Fixes LNK2038)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE INTERNAL "")
# Prevent Jolt from overriding global compiler flags
set(OVERRIDE_CXX_FLAGS OFF CACHE INTERNAL "")
# Enable RTTI and Exceptions for compatibility with EnTT and fastgltf
set(CPP_RTTI_ENABLED ON CACHE INTERNAL "")
set(CPP_EXCEPTIONS_ENABLED ON CACHE INTERNAL "")

# Disable Jolt's internal tests and samples to speed up build
set(TARGET_UNIT_TESTS OFF CACHE INTERNAL "")
set(TARGET_SAMPLES OFF CACHE INTERNAL "")
set(TARGET_HELLO_WORLD OFF CACHE INTERNAL "")
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE ON CACHE INTERNAL "")


include(FetchContent)

# miniaudio
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY "https://github.com/mackron/miniaudio.git"
    GIT_TAG        "0.11.25"
)

# Jolt Physics
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics.git"
    GIT_TAG        "v5.5.0"
    SOURCE_SUBDIR  "Build"
)

# volk (Vulkan loader)
FetchContent_Declare(
    volk
    GIT_REPOSITORY "https://github.com/zeux/volk.git"
    GIT_TAG        "d261d2505214c553a9185f310a7dc822cfadc46b"
)

# VulkanMemoryAllocator
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG        "e722e57c891a8fbe3cc73ca56c19dd76be242759"
)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw.git"
    GIT_TAG        "7c9ea952338b792cb2f70d7c663e498725073797"
)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG        "0af55ccecd98d4e5a8d1fad7de25ba429d60e863"
)

# entt (ECS)
FetchContent_Declare(
    entt
    GIT_REPOSITORY "https://github.com/skypjack/entt.git"
    GIT_TAG        "v3.16.0"
)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG        "v1.17.0"
)

FetchContent_MakeAvailable(
    JoltPhysics volk VulkanMemoryAllocator glfw glm entt spdlog miniaudio
)

# --- Shader Compilation ---
set(SHADER_IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kiki-engine/renderer/shaders)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders/compiled)

file(GLOB SHADERS CONFIGURE_DEPENDS ${SHADER_IN_DIR}/*.vert ${SHADER_IN_DIR}/*.frag ${SHADER_IN_DIR}/*.geom)
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

foreach(shader IN LISTS SHADERS)
    get_filename_component(FILENAME ${shader} NAME)
    set(SPV_FILE ${SHADER_OUT_DIR}/${FILENAME}.spv)
    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
        COMMAND ${GLSLANG_VALIDATOR} -V ${shader} -o ${SPV_FILE}
        DEPENDS ${shader}
        COMMENT "Compiling shader: ${FILENAME}"
    )
    list(APPEND COMPILED_SHADERS ${SPV_FILE})
endforeach()

add_custom_target(shaders ALL DEPENDS ${COMPILED_SHADERS})

# --- KikiDeps Interface ---


# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY "https://github.com/nothings/stb.git"
    GIT_TAG "2fb8c5a3deb2110c89669f8d6f36e5833b556b44"
)
FetchContent_MakeAvailable(stb)

#assimp
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(ASSIMP_ANDROID_JNIIOSYSTEM OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY "https://github.com/assimp/assimp.git"
    GIT_TAG "v6.0.4"
)
FetchContent_MakeAvailable(assimp)

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "6c754ed2cb483488cf728a0d80f0152cbcb5eeb8"
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    # Define which source files to compile
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        # Include the specific backends your engine uses
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )

    # Set the include directories so #include <imgui.h> works everywhere
    target_include_directories(imgui PUBLIC 
        ${imgui_SOURCE_DIR} 
        ${imgui_SOURCE_DIR}/backends
    )

    # Link backends to their respective requirements
    # 'volk' and 'glfw' must be defined before this point
    target_link_libraries(imgui PUBLIC volk glfw spdlog::spdlog)
    
    # If using volk (Vulkan Loader), ImGui needs this definition
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
endif()

# FreeType
FetchContent_Declare(
    freetype
    GIT_REPOSITORY "https://github.com/freetype/freetype.git"
    GIT_TAG        "a6d486002dc6fb26dadfbca45bcab02e1250963a"
)

set(FT_DISABLE_ZLIB  ON CACHE INTERNAL "")
set(FT_DISABLE_BZIP2 ON CACHE INTERNAL "")
set(FT_DISABLE_PNG   ON CACHE INTERNAL "")
set(FT_DISABLE_HARFBUZZ ON CACHE INTERNAL "")
set(FT_DISABLE_BROTLI ON CACHE INTERNAL "")
set(FREETYPE_LIBRARY_TYPE STATIC CACHE INTERNAL "")

FetchContent_MakeAvailable(freetype)

set(TINYXML2_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  tinyxml2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG        9.0.0
)
FetchContent_MakeAvailable(tinyxml2)

FetchContent_Declare(
    msdfgen
    GIT_REPOSITORY "https://github.com/Chlumsky/msdfgen.git"
    GIT_TAG        "da07a9c04f9f92a247fef7da57aa62085647757e"
)

if(TARGET freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()

set(MSDFGEN_INSTALL OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "" FORCE)  
set(MSDFGEN_USE_TINYXML2 OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_LODEPNG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_SVG ON CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(msdfgen)

if(MSVC)
    set_target_properties(msdfgen-core msdfgen-ext PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

set(HB_HAVE_FREETYPE OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GLIB OFF CACHE BOOL "" FORCE)
set(HB_HAVE_ICU OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET ON CACHE BOOL "" FORCE) 

FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG        8.3.0  
)
FetchContent_MakeAvailable(harfbuzz)

# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# -------- Create an interface library to propagate dependencies --------
# This helps your subdirectories automatically get volk, glfw, glm
add_library(KikiDeps INTERFACE)


# tracy
if (ENABLE_TRACY)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY "https://github.com/wolfpld/tracy.git"
        GIT_TAG        "v0.13.1"
    )

    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)

    if (ENABLE_TRACY_VK)
        target_compile_definitions(KikiDeps INTERFACE TRACY_VK_ENABLE)
    endif()

    FetchContent_MakeAvailable(tracy)
    target_link_libraries(KikiDeps INTERFACE Tracy::TracyClient)
endif()

target_link_libraries(KikiDeps INTERFACE 
    volk 
    glfw 
    VulkanMemoryAllocator
    EnTT::EnTT
    spdlog::spdlog
    assimp::assimp
    Jolt
    imgui
    freetype
    msdfgen::msdfgen
    msdfgen::msdfgen-ext
    harfbuzz
)

target_include_directories(KikiDeps INTERFACE 
    ${glm_SOURCE_DIR}
    ${joltphysics_SOURCE_DIR} 
    ${stb_SOURCE_DIR}
    ${msdfgen_SOURCE_DIR}
    ${msdfgen_BINARY_DIR}
    ${miniaudio_SOURCE_DIR}
)
target_compile_definitions(KikiDeps INTERFACE
    JPH_DEBUG_RENDERER
    PROJECT_ROOT_PATH=\"${CMAKE_SOURCE_DIR}\"
    PROJECT_BINARY_DIR=\"${CMAKE_BINARY_DIR}\"
    
    # Synchronize Jolt debug/release modes
    # $<$<CONFIG:Debug>:JPH_DEBUG>
    # $<$<NOT:$<CONFIG:Debug>>:JPH_RELEASE>
    PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}"
    PROJECT_BINARY_DIR="${CMAKE_BINARY_DIR}"
    PROJECT_ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets"
    PROJECT_SHADER_PATH="${CMAKE_BINARY_DIR}/shaders/compiled"
)

# if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
#     target_compile_definitions(KikiDeps INTERFACE JPH_USE_NEON)
# else()
#     target_compile_definitions(KikiDeps INTERFACE JPH_USE_SSE4_1 JPH_USE_SSE4_2 JPH_USE_AVX JPH_USE_AVX2)
# endif()

# --- Subdirectories ---
add_subdirectory(kiki-engine)
add_subdirectory(games/KikiRunner)
add_subdirectory(games/demo)