cmake_minimum_required(VERSION 4.0)

project(Miner)

add_executable(${PROJECT_NAME}
    src/main.cpp src/miner.cpp
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-sodium CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    unofficial-sodium::sodium
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
