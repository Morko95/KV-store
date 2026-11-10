cmake_minimum_required(VERSION 3.20)
project(resp_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(resp_server
    main.cpp
    thread_utils.cpp
    parser.cpp
    input_handlers.cpp
    thread_routine.cpp

)

# 2. Katalogi nagłówkowe
target_include_directories(resp_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_options(resp_server PRIVATE -fsanitize=address,undefined)
target_compile_options(resp_server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fsanitize=address,undefined -g -fno-omit-frame-pointer
)

# 4. Linkowanie bibliotek systemowych: liburing i wątki
find_package(Threads REQUIRED)

target_link_libraries(resp_server PRIVATE
    uring
    Threads::Threads
    xxhash
)
