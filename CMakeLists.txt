# =============================================================================
# CST-310 · topic_09_checkerboard_scene · CMakeLists.txt
# Same structure as topic_01_hello_triangle — see that project's CMakeLists
# for the inline explanation of every block.
# =============================================================================

cmake_minimum_required(VERSION 3.20)
project(topic_09_checkerboard_scene LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /usr/local)
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG QUIET)            # header-only; not required by every project
find_package(OpenGL REQUIRED)

set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glad)
if(NOT EXISTS ${GLAD_DIR}/src/gl.c)
    message(FATAL_ERROR
        "\n  GLAD not found at ${GLAD_DIR}.\n"
        "  Run scripts/setup_mac.sh (Mac) or setup_windows_*.ps1 (Windows) first.\n")
endif()

add_library(glad_local STATIC ${GLAD_DIR}/src/gl.c)
target_include_directories(glad_local PUBLIC ${GLAD_DIR}/include)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad_local OpenGL::GL)
if(TARGET glm::glm)
    target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders next to the executable")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
