cmake_minimum_required(VERSION 3.24)
project(FunDB VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(symengine REQUIRED)
find_package(Catch2 REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(inc)
include_directories(tests)

# Main application
add_executable(my_app 
    src/main.cpp
    src/fun.cpp
    src/db.cpp
)
target_link_libraries(my_app PRIVATE symengine)

# Tests
add_executable(my_app_tests
    tests/tests.cpp
    src/fun.cpp
    src/db.cpp
)
target_link_libraries(my_app_tests PRIVATE symengine Catch2::Catch2WithMain)

# Server executable
add_executable(my_app_server
    src/server.cpp
    src/fun.cpp
    src/db.cpp
)
target_link_libraries(my_app_server PRIVATE symengine nlohmann_json::nlohmann_json)