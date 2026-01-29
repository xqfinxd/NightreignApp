cmake_minimum_required(VERSION 3.12)
project(ImGui)

set(IMGUI_SRC 
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp"
)

add_library(ImGui ${IMGUI_SRC})
target_include_directories(ImGui PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/backends"
)
