cmake_minimum_required(VERSION 3.15)

project(hdf5_archiver)

set(CMAKE_PREFIX_PATH /home/kfeng/install)
find_package(HDF5 REQUIRED COMPONENTS C)

add_executable(compound_datatype_test compound_datatype_test.cpp)

target_link_libraries(compound_datatype_test HDF5::HDF5)
