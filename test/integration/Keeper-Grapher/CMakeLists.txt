cmake_minimum_required(VERSION 3.19)

find_package(Thallium REQUIRED)
find_package(spdlog REQUIRED)

add_custom_target(story_chunk_drain_test)
add_dependencies(story_chunk_drain_test extract_test ingest_test)

add_executable(extract_test)
target_sources(extract_test PRIVATE
    extract_test.cpp
    ../../../chrono_common/chronolog_types.h
    ../../../chrono_common/StoryChunk.cpp
    ../../../chrono_common/ConfigurationManager.cpp
    ../../../ChronoAPI/ChronoLog/src/log.cpp)
target_include_directories(extract_test PRIVATE include
    ../../../chrono_common
    ../../../ChronoAPI/ChronoLog/include)
target_link_libraries(extract_test thallium)
message("build target : extract_test")

add_executable(ingest_test)
target_sources(ingest_test PRIVATE
    ingest_test.cpp
    ../../../chrono_common/chronolog_types.h
    ../../../chrono_common/StoryChunk.cpp
    ../../../chrono_common/ConfigurationManager.cpp
    ../../../ChronoAPI/ChronoLog/src/log.cpp)
target_include_directories(ingest_test PRIVATE include
    ../../../chrono_common
    ../../../ChronoAPI/ChronoLog/include)
target_link_libraries(ingest_test thallium)
message("build target : ingest_test")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../../default_conf.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/default_conf.json COPYONLY)