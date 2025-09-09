cmake_minimum_required(VERSION 3.24)
project(FunDB VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(symengine REQUIRED)

include_directories(inc)

add_executable(my_app 
    src/main.cpp
    src/fun.cpp
    src/db.cpp
)
target_link_libraries(my_app PRIVATE symengine)