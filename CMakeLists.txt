cmake_minimum_required(VERSION 3.16)
project(minigolf C CXX)
set(CMAKE_CXX_STANDARD 17)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# GLFW and GLM: installed with Homebrew, same as in the course assignments
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Dear ImGui: bundled in the repository (third_party/imgui)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
add_library(imgui_lib STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui_lib PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui_lib PUBLIC glfw)

add_executable(minigolf src/main.cpp third_party/glad/src/glad.c)
target_include_directories(minigolf PRIVATE third_party/glad/include src)
target_link_libraries(minigolf PRIVATE glfw glm::glm imgui_lib)
target_compile_definitions(minigolf PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/")
