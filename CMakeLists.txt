
cmake_minimum_required(VERSION 3.16)

project(LANParty VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

add_executable(server
    src/server.cpp
    src/discovery_server.cpp
)

target_link_libraries(server PRIVATE Threads::Threads)

add_executable(client
    src/client.cpp
    src/discovery_client.cpp
)

target_include_directories(client PRIVATE
    ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(client PRIVATE
    Threads::Threads
    ${CURSES_LIBRARIES}
)

add_executable(discovery_client
    src/discovery_client.cpp
)

target_compile_definitions(discovery_client PRIVATE
    DISCOVERY_STANDALONE
)
