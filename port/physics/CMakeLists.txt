# port/physics: Newton 1.x (the 41 entry points src/ imports) implemented over Jolt Physics (MIT), library mb_physics.
# Mapping and semantics: NEWTON_USAGE.md and the header comment of newton_jolt.cpp. Tests: tests/ragdoll_drop.cpp
# (target physics_ragdoll_drop), tests/trace_replay.cpp (target physics_trace_replay: replays a call trace of the real
# Newton 1.53, see trace/RECORDING.md). Jolt is fetched at configure time, pinned to a release tag.
include(FetchContent)

set(MB_JOLT_TAG "v5.5.0" CACHE STRING "Jolt Physics release tag")
set(MB_JOLT_SOURCE_DIR "" CACHE PATH "use a local Jolt checkout instead of fetching (offline builds)")

# Jolt's own options: library only, no LTO, plain float build.
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OVERHEAD OFF CACHE BOOL "" FORCE)
set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)
set(DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)

# Jolt rejects an empty CMAKE_BUILD_TYPE in single-config generators.
set(_mb_saved_build_type "${CMAKE_BUILD_TYPE}")
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(MB_JOLT_SOURCE_DIR)
  add_subdirectory("${MB_JOLT_SOURCE_DIR}/Build" "${CMAKE_CURRENT_BINARY_DIR}/jolt" EXCLUDE_FROM_ALL)
else()
  FetchContent_Declare(JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        ${MB_JOLT_TAG}
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  Build)
  FetchContent_MakeAvailable(JoltPhysics)
endif()
set(CMAKE_BUILD_TYPE "${_mb_saved_build_type}")

add_library(mb_physics STATIC newton_jolt.cpp)
target_compile_features(mb_physics PRIVATE cxx_std_17)
# PRIVATE: only newton_jolt.cpp includes Jolt headers. Jolt's ISA options (-mavx2 -mfma ...) are PUBLIC on its
# target; propagated to the tests they made clang hoist the ragdoll test's fmin updates into wide-XMM VEX code
# that Rosetta mistranslates (pelvisMinZ corrupted to 0 on the first frame). A static lib's PRIVATE deps still
# link into consumers, so the tests keep libJolt.a without the flags.
target_link_libraries(mb_physics PRIVATE Jolt)
set_target_properties(mb_physics PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)

add_executable(physics_ragdoll_drop tests/ragdoll_drop.cpp)
target_compile_features(physics_ragdoll_drop PRIVATE cxx_std_17)
target_link_libraries(physics_ragdoll_drop PRIVATE mb_physics)

# Differential test against the real Newton 1.53: replays a trace recorded from the original game (trace/RECORDING.md)
# and diffs every returned value. ctest (with enable_testing() at the top) runs it on trace/ref/: two attract-mode
# ragdolls, one of them a mounted rider (the mount capsule), replayed open loop, checked for qualitative agreement.
find_package(ZLIB REQUIRED)
add_executable(physics_trace_replay tests/trace_replay.cpp)
target_compile_features(physics_trace_replay PRIVATE cxx_std_17)
target_link_libraries(physics_trace_replay PRIVATE mb_physics ZLIB::ZLIB)
add_test(NAME physics_trace_replay_ref
         COMMAND physics_trace_replay ${CMAKE_CURRENT_SOURCE_DIR}/trace/ref/attract_2ragdolls.bin.gz --quiet)
add_test(NAME physics_ragdoll_drop COMMAND physics_ragdoll_drop)

# The same controlled scenes as trace/probe/newton_experiments.cpp built for Windows against the real Newton.dll
# (trace/build_experiments.sh): run both, diff the outputs.
add_executable(physics_newton_experiments trace/probe/newton_experiments.cpp)
target_link_libraries(physics_newton_experiments PRIVATE mb_physics)
