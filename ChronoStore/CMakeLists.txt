find_package(HDF5 REQUIRED)

include_directories(include)
include_directories(${HDF5_INCLUDE_DIRS})

add_executable(chronostore main.cpp src/storywriter.cpp src/storyreader.cpp)
target_link_libraries(chronostore ${HDF5_LIBRARIES})

add_directory(test)