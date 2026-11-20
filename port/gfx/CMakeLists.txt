# port/gfx/CMakeLists.txt: Direct3D 9 + D3DX (the subset src/ calls) over SDL3's GPU API (tickets 008 / 009).
# Library mb_gfx exports Direct3DCreate9 and the D3DX* functions declared in port/include/d3d9.h / d3dx9.h, so it
# replaces their placeholders in port/stubs/platform_stubs.cpp. Shaders: glslang (HLSL, DX9 compatibility) -> SPIR-V,
# SPIRV-Cross -> MSL on Metal; SPIR-V as is on Vulkan. The window comes from the platform lane: mbp_sdl_window(HWND)
# (port/platform/mb_sdl.h).
find_package(SDL3 CONFIG REQUIRED)

find_path(MBG_GLSLANG_INCLUDE glslang/Include/glslang_c_interface.h HINTS /opt/homebrew/include /usr/local/include /usr/include REQUIRED)
find_path(MBG_SPVC_INCLUDE spirv_cross/spirv_cross_c.h HINTS /opt/homebrew/include /usr/local/include /usr/include REQUIRED)
find_library(MBG_GLSLANG glslang HINTS /opt/homebrew/lib REQUIRED)
find_library(MBG_GLSLANG_LIMITS glslang-default-resource-limits HINTS /opt/homebrew/lib REQUIRED)
set(MBG_SPVC_LIBS "")
foreach(c c msl glsl hlsl cpp reflect util core)
  find_library(MBG_SPVC_${c} spirv-cross-${c} HINTS /opt/homebrew/lib REQUIRED)
  list(APPEND MBG_SPVC_LIBS ${MBG_SPVC_${c}})
endforeach()

set(MBG_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(GLOB MBG_SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl")
add_custom_command(OUTPUT "${MBG_GEN}/gfx_shader_sources.h"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${MBG_GEN}"
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_CURRENT_SOURCE_DIR}/shaders -DOUT=${MBG_GEN}/gfx_shader_sources.h -P ${CMAKE_CURRENT_SOURCE_DIR}/embed_shaders.cmake
  DEPENDS ${MBG_SHADER_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/embed_shaders.cmake"
  COMMENT "embed gfx shaders" VERBATIM)

add_library(mb_gfx STATIC
  d3d9_main.cpp device.cpp device_draw.cpp resources.cpp formats.cpp ffp.cpp shader_compiler.cpp
  effect.cpp asm.cpp d3dx_misc.cpp "${MBG_GEN}/gfx_shader_sources.h")
target_include_directories(mb_gfx PRIVATE ${PORT}/include ${PORT} "${MBG_GEN}" ${MBG_GLSLANG_INCLUDE} ${MBG_SPVC_INCLUDE})
target_compile_options(mb_gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> -fms-extensions -fno-strict-aliasing
  -Wall -Wno-unused-function -Wno-missing-braces -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-microsoft-anon-tag
  -Wno-unused-private-field -Wno-dangling-else -Wno-parentheses)
target_link_libraries(mb_gfx PUBLIC SDL3::SDL3 PRIVATE ${MBG_GLSLANG} ${MBG_GLSLANG_LIMITS} ${MBG_SPVC_LIBS})
set_target_properties(mb_gfx PROPERTIES POSITION_INDEPENDENT_CODE ON)

# tests: each opens a window, exercises the API and writes screenshots (SDL readback) + pixel checks
add_executable(mb_gfx_test tests/gfx_test.cpp)
target_include_directories(mb_gfx_test PRIVATE ${PORT}/include ${PORT})
target_compile_options(mb_gfx_test PRIVATE -std=gnu++17 -fms-extensions -Wno-everything)
target_link_libraries(mb_gfx_test PRIVATE mb_gfx)
