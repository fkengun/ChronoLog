# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/aparna/Documents/filesystems/DHT/micheal

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/aparna/Documents/filesystems/DHT/micheal

# Include any dependencies generated for this target.
include CMakeFiles/block_mem_abt.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/block_mem_abt.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/block_mem_abt.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/block_mem_abt.dir/flags.make

CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o: CMakeFiles/block_mem_abt.dir/flags.make
CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o: block_mem_abt.cpp
CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o: CMakeFiles/block_mem_abt.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/aparna/Documents/filesystems/DHT/micheal/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o"
	mpic++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o -MF CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o.d -o CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o -c /home/aparna/Documents/filesystems/DHT/micheal/block_mem_abt.cpp

CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.i"
	mpic++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/aparna/Documents/filesystems/DHT/micheal/block_mem_abt.cpp > CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.i

CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.s"
	mpic++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/aparna/Documents/filesystems/DHT/micheal/block_mem_abt.cpp -o CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.s

# Object files for target block_mem_abt
block_mem_abt_OBJECTS = \
"CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o"

# External object files for target block_mem_abt
block_mem_abt_EXTERNAL_OBJECTS =

block_mem_abt: CMakeFiles/block_mem_abt.dir/block_mem_abt.cpp.o
block_mem_abt: CMakeFiles/block_mem_abt.dir/build.make
block_mem_abt: /home/aparnasasidharan/Documents/lib/libmpicxx.so
block_mem_abt: /home/aparnasasidharan/Documents/lib/libmpi.so
block_mem_abt: /home/aparna/spack/opt/spack/linux-ubuntu22.04-skylake/gcc-11.2.0/boost-1.79.0-3723mcqaxdcv3ncdxxt533su7rreljjc/lib/libboost_atomic.so.1.79.0
block_mem_abt: /home/aparna/spack/opt/spack/linux-ubuntu22.04-skylake/gcc-11.2.0/argobots-1.1-3tu2qifr4rgoxks4h5bo64vmmjkdt3w5/lib/libabt.so
block_mem_abt: CMakeFiles/block_mem_abt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/aparna/Documents/filesystems/DHT/micheal/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable block_mem_abt"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/block_mem_abt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/block_mem_abt.dir/build: block_mem_abt
.PHONY : CMakeFiles/block_mem_abt.dir/build

CMakeFiles/block_mem_abt.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/block_mem_abt.dir/cmake_clean.cmake
.PHONY : CMakeFiles/block_mem_abt.dir/clean

CMakeFiles/block_mem_abt.dir/depend:
	cd /home/aparna/Documents/filesystems/DHT/micheal && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/aparna/Documents/filesystems/DHT/micheal /home/aparna/Documents/filesystems/DHT/micheal /home/aparna/Documents/filesystems/DHT/micheal /home/aparna/Documents/filesystems/DHT/micheal /home/aparna/Documents/filesystems/DHT/micheal/CMakeFiles/block_mem_abt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/block_mem_abt.dir/depend
