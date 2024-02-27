cmake_minimum_required(VERSION 3.17)
find_package(HDF5 REQUIRED)

include_directories(../include)
include_directories(${HDF5_INCLUDE_DIRS})

add_executable(chronostore_test testreadwrite.cpp ../include/storywriter.h ../src/storywriter.cpp ../include/storyreader.h ../src/storyreader.cpp)

target_link_libraries(chronostore_test ${HDF5_LIBRARIES})
add_test(NAME chronostore_test COMMAND chronostore_test)