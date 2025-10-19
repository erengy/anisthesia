cmake_minimum_required(VERSION 3.20)

project(anisthesia
	VERSION 1.0.0
	DESCRIPTION "Media detection library"
	HOMEPAGE_URL https://github.com/erengy/anisthesia
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(anisthesia INTERFACE)

target_include_directories(anisthesia INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(anisthesia INTERFACE
	src/matroska.cpp
	src/player.cpp
	src/util.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(anisthesia INTERFACE
		windowsapp.lib
	)

	target_sources(anisthesia INTERFACE
		src/win_open_files.cpp
		src/win_platform.cpp
		src/win_smtc.cpp
		src/win_strategies.cpp
		src/win_ui_automation.cpp
		src/win_util.cpp
		src/win_windows.cpp
	)
endif()
