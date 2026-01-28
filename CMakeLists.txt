cmake_minimum_required(VERSION 3.12)

project(NightreignApp)

set(CMAKE_CXX_STANDARD 17)

add_executable(NightreignApp 
	src/ThirdParty.cpp
	src/Main.cpp
)

target_include_directories(NightreignApp PRIVATE 
	3rdparty/glm/include
	3rdparty/stb_image
)

if(EMSCRIPTEN)
	target_compile_options(NightreignApp PRIVATE 
		-sUSE_SDL=2
		-sUSE_SDL_TTF=2
		-O3
		-lidbfs.js
	)
	target_link_options(NightreignApp PRIVATE 
		-sUSE_SDL=2
		-sUSE_SDL_TTF=2
		-sWASM=1
		-sMIN_WEBGL_VERSION=2
		-sMAX_WEBGL_VERSION=2
		-sALLOW_MEMORY_GROWTH=1
		-O3
		-lidbfs.js
	)
	target_compile_definitions(NightreignApp PRIVATE 
	)
else()
	add_subdirectory(3rdparty/glad)
	add_subdirectory(3rdparty/IMGUI)
	find_package(SDL2 REQUIRED)
	find_package(SDL2_TTF REQUIRED)
	target_link_libraries(NightreignApp PRIVATE
		SDL2::SDL2
		SDL2_ttf::SDL2_ttf
		glad
		ImGui
	)
	target_include_directories(NightreignApp PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL2_TTF_INCLUDE_DIRS}
	)
	target_compile_definitions(NightreignApp PRIVATE 
		SDL_MAIN_HANDLED
	)
	target_compile_options(NightreignApp PRIVATE
		/utf-8
	)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()
