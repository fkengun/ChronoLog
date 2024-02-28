find_package(HDF5 REQUIRED)

include_directories(include
    ../ChronoAPI/ChronoLog/include
    ../ChronoKeeper)

option(BUILD_TEST_DIRECTORY "Build Test Directory" ON)
if(BUILD_TEST_DIRECTORY)
    add_subdirectory(test)
endif()

include_directories(${HDF5_INCLUDE_DIRS})
include_directories(${JSON-C_INCLUDE_DIRS})

add_executable(chronostore main.cpp src/StoryWriter.cpp src/StoryReader.cpp)
target_include_directories(chronostore PRIVATE ../ChronoKeeper
    ../chrono_common
    ./include)
target_link_libraries(chronostore ${HDF5_LIBRARIES} json-c)