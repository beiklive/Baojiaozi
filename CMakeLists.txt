cmake_minimum_required(VERSION 3.25)

project(Baojiaozi VERSION 0.1.0 LANGUAGES CXX)

option(BAOJIAOZI_BUILD_DESIGNER "Build the standalone ImGui designer" ON)
option(BAOJIAOZI_BUILD_TESTS "Build Baojiaozi tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw EXCLUDE_FROM_ALL)

add_library(baojiaozi_core STATIC
    src/version.cpp
)
target_include_directories(baojiaozi_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(baojiaozi_imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(baojiaozi_imgui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
)
target_link_libraries(baojiaozi_imgui PUBLIC glfw)

find_package(OpenGL REQUIRED)

if(BAOJIAOZI_BUILD_DESIGNER)
    add_executable(baojiaozi_designer
        designer/main.cpp
    )
    target_compile_definitions(baojiaozi_designer PRIVATE
        BAOJIAOZI_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        GL_SILENCE_DEPRECATION
    )
    target_link_libraries(baojiaozi_designer PRIVATE
        baojiaozi_core
        baojiaozi_imgui
        OpenGL::GL
    )
endif()

if(BAOJIAOZI_BUILD_TESTS)
    enable_testing()
    add_executable(baojiaozi_core_tests
        tests/core_smoke.cpp
    )
    target_link_libraries(baojiaozi_core_tests PRIVATE baojiaozi_core)
    add_test(NAME baojiaozi_core_tests COMMAND baojiaozi_core_tests)
endif()
