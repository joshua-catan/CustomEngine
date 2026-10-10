
cmake_minimum_required(VERSION 3.20)

project(MyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/external/imgui")

add_executable(MyEngine
    src/main.cpp
    src/engine/Scene.cpp
    src/engine/Renderer.cpp

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(MyEngine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_link_libraries(MyEngine PRIVATE
    glfw
    OpenGL::GL
    GLEW::GLEW
)