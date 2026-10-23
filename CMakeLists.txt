cmake_minimum_required(VERSION 3.24)
project(ErScripts LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.h" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/single_include/nlohmann/json.hpp" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/httplib/httplib.h")
    message(FATAL_ERROR "Run: git submodule update --init --recursive")
endif()
add_library(imgui STATIC
    third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC third_party/imgui)
if(WIN32)
    target_sources(imgui PRIVATE third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
    file(GLOB ERSCRIPTS_SOURCES CONFIGURE_DEPENDS ErScripts/*.cpp)
    add_executable(ErScripts ${ERSCRIPTS_SOURCES} ErScripts/UIAccess.c ErScripts/ErScripts.rc)
    target_include_directories(ErScripts PRIVATE ErScripts third_party/json/single_include third_party/httplib)
    target_compile_definitions(ErScripts PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(ErScripts PRIVATE imgui d3d11 d3dcompiler dxgi dwmapi windowscodecs winmm winhttp ws2_32 crypt32 advapi32 user32 gdi32 ole32 shell32 comdlg32)
    if(MSVC)
        target_compile_options(ErScripts PRIVATE /utf-8 /W3)
    endif()
endif()
