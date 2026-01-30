cmake_minimum_required(VERSION 3.12)

project(NightreignApp)

set(CMAKE_CXX_STANDARD 17)

add_executable(NightreignApp 
	"src/ThirdParty.cpp"
	"src/Application.cpp"
	"src/Main.cpp"
	"src/Window.cpp"
	"src/Device.cpp"
	"src/Renderer.cpp")

target_include_directories(NightreignApp PRIVATE 
	"3rdparty/glm/include"
	"3rdparty/stb_image"
)

target_link_libraries(NightreignApp PRIVATE
	ImGui
)

add_subdirectory(3rdparty/IMGUI)

if(EMSCRIPTEN)
	target_compile_options(NightreignApp PRIVATE 
		-sUSE_SDL=2
#		-sUSE_SDL_TTF=2
		-O3
		-lidbfs.js
	)
	target_link_options(NightreignApp PRIVATE 
		-sUSE_SDL=2
#		-sUSE_SDL_TTF=2
		-sWASM=1
		-sMIN_WEBGL_VERSION=2
		-sMAX_WEBGL_VERSION=2
		-sALLOW_MEMORY_GROWTH=1
		-O3
		-lidbfs.js
	)
	target_compile_definitions(NightreignApp PRIVATE 
	)
elseif(MSVC)
	add_subdirectory(3rdparty/glad)
	find_package(SDL2 REQUIRED PATHS 3rdparty/SDL2)
#	find_package(SDL2_TTF REQUIRED  PATHS 3rdparty/SDL2_ttf)
	target_link_libraries(NightreignApp PRIVATE
		SDL2::SDL2
#		SDL2_ttf::SDL2_ttf
		glad
	)
	target_include_directories(NightreignApp PRIVATE
		${SDL2_INCLUDE_DIRS}
#		${SDL2_TTF_INCLUDE_DIRS}
	)
	target_compile_definitions(NightreignApp PRIVATE 
		SDL_MAIN_HANDLED
	)
	target_compile_options(NightreignApp PRIVATE
		/utf-8
	)
	file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
	file(COPY ${SDL2_BINDIR}/SDL2.dll DESTINATION ${CMAKE_BINARY_DIR})
endif()
