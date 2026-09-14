cmake_minimum_required(VERSION 3.24)
project(thprac_native LANGUAGES C CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "This target builds the x64 game module; build the existing launcher with thprac.sln")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(core "${CMAKE_CURRENT_SOURCE_DIR}/thprac/src/thprac")
set(vendor "${CMAKE_CURRENT_SOURCE_DIR}/thprac/src/3rdParties")
add_subdirectory("${vendor}/MinHook")
add_library(thprac_th06nc SHARED
  ${core}/th06nc/runtime.cpp ${core}/th06nc/overlay.cpp ${core}/th06nc/ecl.cpp ${core}/th06nc/presentation.cpp
  ${core}/thprac_gui_components.cpp ${core}/thprac_gui_input.cpp
  ${core}/thprac_gui_locale.cpp ${core}/thprac_gui_impl_win32.cpp
  ${core}/thprac_locale_def.cpp
  ${vendor}/ImGui/imgui.cpp ${vendor}/ImGui/imgui_draw.cpp
  ${vendor}/ImGui/imgui_widgets.cpp ${vendor}/ImGui/imgui_tables.cpp
  ${vendor}/ImGui/imgui_freetype.cpp ${vendor}/ImGui/imgui_impl_dx11.cpp)
target_include_directories(thprac_th06nc PRIVATE ${core} ${vendor}/ImGui ${vendor}/FreeType/include)
target_compile_definitions(thprac_th06nc PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE THPRAC_NATIVE_WIDGETS_ONLY)
target_compile_options(thprac_th06nc PRIVATE /utf-8 /Gy /EHsc)
target_link_options(thprac_th06nc PRIVATE /OPT:REF /OPT:ICF)
target_link_libraries(thprac_th06nc PRIVATE minhook "${vendor}/FreeType/win64/freetype.lib" d3d11 dxgi d3dcompiler bcrypt user32 gdi32 imm32)
add_executable(thprac_bridge64 WIN32 ${core}/th06nc/bridge.cpp)
target_compile_definitions(thprac_bridge64 PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(thprac_bridge64 PRIVATE /utf-8 /EHsc)
target_link_libraries(thprac_bridge64 PRIVATE bcrypt shell32 user32)
add_custom_command(TARGET thprac_th06nc POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${vendor}/FreeType/win64/freetype.dll" "$<TARGET_FILE_DIR:thprac_th06nc>/freetype.dll")
