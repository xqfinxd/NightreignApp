cmake_minimum_required(VERSION 3.12)

project(NightreignApp)

set(CMAKE_CXX_STANDARD 17)

add_executable(NightreignApp 
	"src/ThirdParty.cpp"
	"src/Application.cpp"
	"src/Main.cpp"
	"src/Renderer.cpp"
	"src/Scene.cpp"
	"src/SceneManager.cpp"
	"src/Shader.cpp"
	"src/ResourceManager.cpp"
	"src/AsyncResourceLoader.cpp"
	"src/TextureRegistry.cpp"
	"src/Buffer.cpp"
	"src/Texture.cpp"
	"src/sdlWindow.cpp"
	"src/glDevice.cpp"
	"src/CsvReader.cpp"
	"src/GameData.cpp"
	"src/JSBindings.cpp")

target_include_directories(NightreignApp PRIVATE
	"src"
	"3rdparty/glm/include"
	"3rdparty/stb_image"
	"3rdparty/entt"
	"3rdparty/sqlite3"
	"3rdparty/RapidJSON/include"
)

target_link_libraries(NightreignApp PRIVATE
	ImGui
	sqlite3
)

add_subdirectory(3rdparty/IMGUI)
add_subdirectory(3rdparty/sqlite3)

if(EMSCRIPTEN)
	target_compile_options(NightreignApp PRIVATE 
		-sUSE_SDL=2
#		-sUSE_SDL_TTF=2
		-O3
	)
	
	target_link_options(NightreignApp PRIVATE 
		-sUSE_SDL=2
#		-sUSE_SDL_TTF=2
		-sWASM=1
		-sMIN_WEBGL_VERSION=2
		-sMAX_WEBGL_VERSION=2
		-sALLOW_MEMORY_GROWTH=1
		-sFORCE_FILESYSTEM=1
		-lidbfs.js
		-sEXPORTED_RUNTIME_METHODS=[FS,ccall,cwrap]
		-sFETCH=1
		-lembind
		-O3
	)
	target_compile_definitions(NightreignApp PRIVATE 
	)
	add_custom_command(TARGET NightreignApp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/nightreign
			${CMAKE_SOURCE_DIR}/deploy/nightreign
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:NightreignApp>
			${CMAKE_SOURCE_DIR}/deploy/nightreign/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE_DIR:NightreignApp>/$<TARGET_FILE_BASE_NAME:NightreignApp>.wasm
			${CMAKE_SOURCE_DIR}/deploy/nightreign/
	)
elseif(MSVC)
	add_subdirectory(3rdparty/glad)
	add_subdirectory(3rdparty/lua)
	find_package(SDL2 REQUIRED PATHS 3rdparty/SDL2)
#	find_package(SDL2_TTF REQUIRED  PATHS 3rdparty/SDL2_ttf)
	target_link_libraries(NightreignApp PRIVATE
		SDL2::SDL2
#		SDL2_ttf::SDL2_ttf
		glad
		lua
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
	# Create nightreign directory in output executable dir and copy assets
	add_custom_command(TARGET NightreignApp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/nightreign
		$<TARGET_FILE_DIR:NightreignApp>/nightreign
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${SDL2_BINDIR}/SDL2.dll"
		$<TARGET_FILE_DIR:NightreignApp>)
endif()
