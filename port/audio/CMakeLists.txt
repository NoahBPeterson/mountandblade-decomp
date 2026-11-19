# port/audio/CMakeLists.txt: FMOD 3.75 (the 26 entry points src/ imports) over SDL3 audio + stb_vorbis (ticket 010).
# Library mb_audio exports the FSOUND_* names (extern "C", stdcall on i686) so it replaces port/stubs/fmod_stubs.cpp.
find_package(SDL3 CONFIG REQUIRED)

add_library(mb_stb_vorbis STATIC third_party/stb_vorbis.c)
target_compile_options(mb_stb_vorbis PRIVATE -w)
target_compile_definitions(mb_stb_vorbis PRIVATE STB_VORBIS_NO_PUSHDATA_API)
set_target_properties(mb_stb_vorbis PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(mb_audio STATIC fmod_sdl.cpp wav.cpp)
target_include_directories(mb_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(mb_audio PRIVATE cxx_std_17)
target_link_libraries(mb_audio PUBLIC SDL3::SDL3 PRIVATE mb_stb_vorbis)
find_package(Threads)
if(Threads_FOUND)
  target_link_libraries(mb_audio PRIVATE Threads::Threads)
endif()

# Standalone test: plays a game sample + music stream through the FSOUND API and renders the mix to a WAV.
add_executable(mb_audio_test tests/audio_test.cpp)
target_link_libraries(mb_audio_test PRIVATE mb_audio)

add_executable(mb_audio_bench tests/audio_bench.cpp)
target_link_libraries(mb_audio_bench PRIVATE mb_audio)
