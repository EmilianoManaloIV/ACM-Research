cmake_minimum_required(VERSION 3.20)
project(ACMResearch LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
# X11 only by default (works under XWayland too); pass -DGLFW_BUILD_WAYLAND=ON
# for native Wayland, which needs the wayland-scanner and libwayland dev packages.
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Build GLFW with native Wayland support")
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW TRUE)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(glfw glm)

find_package(OpenGL REQUIRED)

# OpenGL 3.3 core loader, generated with glad2 (see third_party/README.md).
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# Shared research code: windowing, shaders, image capture.
add_library(acm
  src/acm/window.cpp
  src/acm/shader.cpp
  src/acm/capture.cpp)
target_include_directories(acm PUBLIC src)
target_link_libraries(acm PUBLIC glad glfw glm::glm OpenGL::GL)
target_compile_definitions(acm PUBLIC ACM_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")
if(MSVC)
  target_compile_options(acm PRIVATE /W4)
else()
  target_compile_options(acm PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(apps/sandbox)
