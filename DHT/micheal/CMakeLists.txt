cmake_minimum_required(VERSION 3.10)

project(micheal)

set(CMAKE_CXX_STANDARD 17)

message(${CMAKE_SOURCE_DIR})
set(CMAKE_C_COMPILER mpicc)
set(CMAKE_CXX_COMPILER mpic++)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(block_srcs block_mem.cpp)
add_executable(block_mem ${block_srcs})
set(block_srcs_abt block_mem_abt.cpp)
add_executable(block_mem_abt ${block_srcs_abt})
set(block_srcs_dist block_mem_dist.cpp)
add_executable(block_mem_dist ${block_srcs_dist})

find_package(Boost COMPONENTS ALL)
include_directories(${Boost_INCLUDE_DIRS})
find_package(MPI REQUIRED COMPONENTS CXX C)
include_directories(${MPI_CXX_INCLUDE_DIRS})
find_package(Argobots COMPONENTS ALL)
include_directories(${Argobots_INCLUDE_DIRS})

set(LIB_FLAGS ${MPI_CXX_LIBRARIES} Boost::atomic -lpthread -lrt)

# libs for NATS tests
#set(LIB_FLAGS ${LIB_FLAGS} -lnats)

# libs for Redis tests
#set(LIB_FLAGS ${LIB_FLAGS} -lev -L/home/kbateman/install/lib/ -L/home/kbateman/install/lib64/ -lhiredis -lredox)

message(${LIB_FLAGS})
target_link_libraries(block_mem ${LIB_FLAGS})
target_link_libraries(block_mem_abt ${LIB_FLAGS} ${Argobots_LIBRARIES})
target_link_libraries(block_mem_dist ${LIB_FLAGS})
