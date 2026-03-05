project(GI_RENDERER)
cmake_minimum_required(VERSION 4.1)
set(CMAKE_CXX_STANDARD 20)

include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/include)

link_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/lib)


file(GLOB ASSETS "./assets")
file(COPY ${ASSETS} DESTINATION ${CMAKE_BINARY_DIR})

add_subdirectory(wrapper)
add_subdirectory(engine)
add_subdirectory(tools)

add_executable(GI_RENDERER glad.c run.cpp)
target_link_libraries(GI_RENDERER glfw3.lib assimp-vc143-mtd.lib zlibstaticd.lib engine wrapper tools)