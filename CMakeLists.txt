cmake_minimum_required(VERSION 3.20)

project(MacAltWinSwap VERSION 0.1.0 LANGUAGES CXX RC)

add_executable(MacAltWinSwap WIN32
    src/app.rc
    src/main.cpp
)

target_compile_features(MacAltWinSwap PRIVATE cxx_std_17)
target_compile_definitions(MacAltWinSwap PRIVATE UNICODE _UNICODE)
target_link_libraries(MacAltWinSwap PRIVATE user32 shell32 advapi32)

if(MINGW)
    target_link_options(MacAltWinSwap PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
