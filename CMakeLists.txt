cmake_minimum_required(VERSION 3.24)
project(ErScripts LANGUAGES C CXX)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.h" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/single_include/nlohmann/json.hpp" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/httplib/httplib.h")
    message(FATAL_ERROR "Run: git submodule update --init --recursive")
endif()
add_library(imgui STATIC
    third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp)
target_include_directories(imgui SYSTEM PUBLIC third_party/imgui)
if(WIN32)
    target_sources(imgui PRIVATE third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
    set(ERSCRIPTS_SOURCES
        src/app/application.cpp
        src/features/features.cpp
        src/game/game_console.cpp
        src/game/game_window.cpp
        src/features/anti_afk.cpp
        src/features/auto_accept.cpp
        src/features/auto_pistol.cpp
        src/features/auto_stop.cpp
        src/features/bomb_timer.cpp
        src/features/chat_spammer.cpp
        src/core/config.cpp
        src/core/config_export.cpp
        src/features/crosshair.cpp
        src/features/cs2_binds.cpp
        src/platform/file_monitor.cpp
        src/core/globals.cpp
        src/core/color_gradient.cpp
        src/game/gsi_server.cpp
        src/game/gsi_state.cpp
        src/platform/input.cpp
        src/features/kill_say.cpp
        src/features/kill_sound.cpp
        src/features/knife_switch.cpp
        src/core/logger.cpp
        src/graphics/menu.cpp
        src/graphics/overlay.cpp
        src/graphics/overlay_drawing.cpp
        src/features/pixel_trigger.cpp
        src/graphics/render.cpp
        src/features/rgb_crosshair.cpp
        src/features/round_start_alert.cpp
        src/platform/simple_sound.cpp
        src/game/steam_tools.cpp
        src/update/updater.cpp
    )
    add_library(erscripts_core STATIC ${ERSCRIPTS_SOURCES} src/platform/ui_access.c)
    target_compile_features(erscripts_core PUBLIC cxx_std_20)
    set_target_properties(erscripts_core PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(erscripts_core PUBLIC src)
    target_include_directories(erscripts_core SYSTEM PUBLIC third_party/json/single_include third_party/httplib)
    target_compile_definitions(erscripts_core PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(erscripts_core PUBLIC imgui d3d11 d3dcompiler dxgi dwmapi windowscodecs winmm winhttp bcrypt ws2_32 crypt32 advapi32 user32 gdi32 ole32 shell32 comdlg32)
    add_executable(ErScripts src/app/main.cpp resources/erscripts.rc)
    target_include_directories(ErScripts PRIVATE resources)
    target_link_libraries(ErScripts PRIVATE erscripts_core)
    if(MSVC)
        target_link_options(ErScripts PRIVATE "$<$<NOT:$<CONFIG:Debug>>:/MANIFESTUAC:level='requireAdministrator' uiAccess='false'>")
        target_compile_options(erscripts_core PUBLIC /utf-8 /W4 /permissive- /external:W0)
    endif()
    include(CTest)
    if(BUILD_TESTING)
        add_executable(ErScriptsTests tests/er_scripts_tests.cpp)
        target_link_libraries(ErScriptsTests PRIVATE erscripts_core)
        add_test(NAME ErScriptsTests COMMAND ErScriptsTests)
        add_executable(UpdaterTests tests/updater_tests.cpp)
        target_link_libraries(UpdaterTests PRIVATE erscripts_core)
        add_test(NAME UpdaterTests COMMAND UpdaterTests)
        add_executable(GameConsoleTests tests/game_console_tests.cpp)
        target_link_libraries(GameConsoleTests PRIVATE erscripts_core)
        add_test(NAME GameConsoleTests COMMAND GameConsoleTests)
        set_tests_properties(ErScriptsTests UpdaterTests GameConsoleTests PROPERTIES TIMEOUT 90)
    endif()
endif()
