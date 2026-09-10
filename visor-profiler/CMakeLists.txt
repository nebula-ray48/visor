include(FetchContent)

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.91.9b
	GIT_SHALLOW TRUE
)
FetchContent_Declare(
	implot
	GIT_REPOSITORY https://github.com/epezent/implot.git
	GIT_TAG v0.16
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui implot)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMPLOT_DIR ${implot_SOURCE_DIR})

add_library(imgui STATIC
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)

add_library(implot STATIC
	${IMPLOT_DIR}/implot.cpp
	${IMPLOT_DIR}/implot_items.cpp
)
target_include_directories(implot PUBLIC ${IMPLOT_DIR})
target_link_libraries(implot PUBLIC imgui)

add_executable(visor_profiler src/main.cpp)
target_include_directories(visor_profiler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(visor_profiler PRIVATE implot imgui Vulkan::Vulkan glfw)
