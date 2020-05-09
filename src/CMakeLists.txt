cmake_minimum_required (VERSION 3.4)
project (AFPACKET)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -Wextra -pedantic-errors -Wdeprecated -Wshadow")

find_package (Threads)

add_executable (afpacket_test.exe main.cpp packets.cpp util.cpp)
target_link_libraries (afpacket_test.exe ${CMAKE_THREAD_LIBS_INIT})