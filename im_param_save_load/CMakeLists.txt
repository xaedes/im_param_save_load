cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME im_param_save_load)
project( ${PROJECT_NAME} )


option(WITH_IMGUI_BACKEND "Build with ImGui backend." ON)
option(BUILD_EXAMPLE "Build example." ON)

include("compiler_options.cmake")

add_library(
    ${PROJECT_NAME} 
    INTERFACE
)

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
    
target_link_libraries(${PROJECT_NAME} INTERFACE im_param)

if (WITH_IMGUI_BACKEND)
    find_package(imgui REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE imgui::imgui)
    target_link_libraries(${PROJECT_NAME} INTERFACE imgui_candy)
endif()

# if (BUILD_EXAMPLE)
#     add_executable(${PROJECT_NAME}_example src/example.cpp)
#     target_link_libraries(${PROJECT_NAME}_example PRIVATE ${PROJECT_NAME})
# endif()
