cmake_minimum_required(VERSION 3.20)
project(MusicPlayer LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "This project currently targets Windows.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Use a 64-bit compiler to match the bundled BASS library.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v6.1.9
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ftxui)

add_library(bass SHARED IMPORTED)
set_target_properties(bass PROPERTIES
  IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/Header_Files/bass.lib"
  IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bass.dll"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/Header_Files"
)

function(configure_player target)
  target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -finput-charset=UTF-8)
  endif()
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/bass.dll" "$<TARGET_FILE_DIR:${target}>"
  )
  if(MINGW)
    foreach(runtime libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
      execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${runtime}
        OUTPUT_VARIABLE runtime_path OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(NOT EXISTS "${runtime_path}")
        get_filename_component(compiler_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(runtime_path "${compiler_directory}/${runtime}")
      endif()
      if(EXISTS "${runtime_path}")
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E
          copy_if_different "${runtime_path}" "$<TARGET_FILE_DIR:${target}>")
      endif()
    endforeach()
  endif()
endfunction()

add_executable(music_player main.cpp)
target_link_libraries(music_player PRIVATE bass ftxui::component ftxui::dom ftxui::screen)
configure_player(music_player)
if(MINGW)
  target_link_options(music_player PRIVATE -municode)
endif()

