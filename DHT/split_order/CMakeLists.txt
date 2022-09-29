cmake_minimum_required(VERSION 3.10)

project(nir)

set(CMAKE_CXX_STANDARD 17)

message(${CMAKE_SOURCE_DIR})
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(split_srcs split_mem.cpp)
add_executable(split_mem ${split_srcs})

find_package(Boost COMPONENTS ALL)
include_directories(${Boost_INCLUDE_DIRS})
find_package(MPI REQUIRED COMPONENTS CXX C)
include_directories(${MPI_CXX_INCLUDE_DIRS})

set(LIB_FLAGS ${MPI_CXX_LIBRARIES} Boost::boost Boost::thread Boost::atomic -lpthread -lrt)

# libs for NATS tests
#set(LIB_FLAGS ${LIB_FLAGS} -lnats)

# libs for Redis tests
#set(LIB_FLAGS ${LIB_FLAGS} -lev -L/home/kbateman/install/lib/ -L/home/kbateman/install/lib64/ -lhiredis -lredox)

message(${LIB_FLAGS})
target_link_libraries(split_mem ${LIB_FLAGS})
