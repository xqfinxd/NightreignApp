cmake_minimum_required(VERSION 3.12)
project(GLAD)

add_library(glad ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)