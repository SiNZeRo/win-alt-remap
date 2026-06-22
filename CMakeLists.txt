cmake_minimum_required(VERSION 3.20)

project(WinAltRemap VERSION 0.1.0 LANGUAGES CXX RC)

add_executable(WinAltRemap WIN32
    src/app.rc
    src/main.cpp
)

target_compile_features(WinAltRemap PRIVATE cxx_std_17)
target_compile_definitions(WinAltRemap PRIVATE UNICODE _UNICODE)
target_link_libraries(WinAltRemap PRIVATE user32 shell32 advapi32)

if(MINGW)
    target_link_options(WinAltRemap PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
