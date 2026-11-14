cmake_minimum_required(VERSION 3.24)
project(ErScripts LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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
target_include_directories(imgui PUBLIC third_party/imgui)
if(WIN32)
    target_sources(imgui PRIVATE third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
    set(ERSCRIPTS_SOURCES
        src/app/Application.cpp
        src/features/Features.cpp
        src/game/GameConsole.cpp
        src/game/GameWindow.cpp
        src/features/AntiAfk.cpp
        src/features/AutoAccept.cpp
        src/features/AutoPistol.cpp
        src/features/AutoStop.cpp
        src/features/BombTime.cpp
        src/features/ChatSpammer.cpp
        src/core/Config.cpp
        src/core/ConfigExport.cpp
        src/features/Crosshair.cpp
        src/features/CS2Binds.cpp
        src/platform/FileMonitor.cpp
        src/core/Globals.cpp
        src/core/GradientManager.cpp
        src/game/GSIServer.cpp
        src/game/GSIState.cpp
        src/platform/Input.cpp
        src/features/KillSay.cpp
        src/features/KillSound.cpp
        src/features/KnifeSwitch.cpp
        src/core/Logger.cpp
        src/graphics/Menu.cpp
        src/graphics/Overlay.cpp
        src/graphics/OverlayHelper.cpp
        src/features/PixelTrigger.cpp
        src/graphics/Render.cpp
        src/features/RGBCrosshair.cpp
        src/features/RoundStartAlert.cpp
        src/platform/SimpleSound.cpp
        src/game/SteamTools.cpp
        src/update/Updater.cpp
    )
    add_library(erscripts_core STATIC ${ERSCRIPTS_SOURCES} src/platform/UIAccess.c)
    target_include_directories(erscripts_core PUBLIC src third_party/json/single_include third_party/httplib)
    target_compile_definitions(erscripts_core PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(erscripts_core PUBLIC imgui d3d11 d3dcompiler dxgi dwmapi windowscodecs winmm winhttp bcrypt ws2_32 crypt32 advapi32 user32 gdi32 ole32 shell32 comdlg32)
    add_executable(ErScripts src/app/main.cpp resources/ErScripts.rc)
    target_include_directories(ErScripts PRIVATE resources)
    target_link_libraries(ErScripts PRIVATE erscripts_core)
    if(MSVC)
        target_link_options(ErScripts PRIVATE "$<$<NOT:$<CONFIG:Debug>>:/MANIFESTUAC:level='requireAdministrator' uiAccess='false'>")
        target_compile_options(erscripts_core PUBLIC /utf-8 /W4 /external:anglebrackets /external:W0)
    endif()
    include(CTest)
    if(BUILD_TESTING)
        add_executable(ErScriptsTests tests/ErScriptsTests.cpp)
        target_link_libraries(ErScriptsTests PRIVATE erscripts_core)
        add_test(NAME ErScriptsTests COMMAND ErScriptsTests)
        add_executable(UpdaterTests tests/UpdaterTests.cpp)
        target_link_libraries(UpdaterTests PRIVATE erscripts_core)
        add_test(NAME UpdaterTests COMMAND UpdaterTests)
        add_executable(GameConsoleTests tests/GameConsoleTests.cpp)
        target_link_libraries(GameConsoleTests PRIVATE erscripts_core)
        add_test(NAME GameConsoleTests COMMAND GameConsoleTests)
        set_tests_properties(ErScriptsTests UpdaterTests GameConsoleTests PROPERTIES TIMEOUT 90)
    endif()
endif()
