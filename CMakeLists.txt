cmake_minimum_required(VERSION 3.24)
project(visor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VISOR_BUILD_ENGINE_MOCK "Build the shared-memory engine mock" ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_subdirectory(visor-profiler)

if(VISOR_BUILD_ENGINE_MOCK)
	add_subdirectory(engine-mock)
endif()
