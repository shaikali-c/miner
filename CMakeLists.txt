cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

project(Miner)

add_executable(${PROJECT_NAME}
    src/main.cpp src/miner.cpp include/miner.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE sodium cpr)
target_include_directories(${PROJECT_NAME} PRIVATE include)
