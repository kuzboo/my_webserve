# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/yhb/test/my_webServer/test

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/yhb/test/my_webServer/test/build

# Include any dependencies generated for this target.
include CMakeFiles/testmysql.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/testmysql.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/testmysql.dir/flags.make

CMakeFiles/testmysql.dir/testmysql.cpp.o: CMakeFiles/testmysql.dir/flags.make
CMakeFiles/testmysql.dir/testmysql.cpp.o: ../testmysql.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/yhb/test/my_webServer/test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/testmysql.dir/testmysql.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/testmysql.dir/testmysql.cpp.o -c /home/yhb/test/my_webServer/test/testmysql.cpp

CMakeFiles/testmysql.dir/testmysql.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/testmysql.dir/testmysql.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/yhb/test/my_webServer/test/testmysql.cpp > CMakeFiles/testmysql.dir/testmysql.cpp.i

CMakeFiles/testmysql.dir/testmysql.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/testmysql.dir/testmysql.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/yhb/test/my_webServer/test/testmysql.cpp -o CMakeFiles/testmysql.dir/testmysql.cpp.s

CMakeFiles/testmysql.dir/testmysql.cpp.o.requires:

.PHONY : CMakeFiles/testmysql.dir/testmysql.cpp.o.requires

CMakeFiles/testmysql.dir/testmysql.cpp.o.provides: CMakeFiles/testmysql.dir/testmysql.cpp.o.requires
	$(MAKE) -f CMakeFiles/testmysql.dir/build.make CMakeFiles/testmysql.dir/testmysql.cpp.o.provides.build
.PHONY : CMakeFiles/testmysql.dir/testmysql.cpp.o.provides

CMakeFiles/testmysql.dir/testmysql.cpp.o.provides.build: CMakeFiles/testmysql.dir/testmysql.cpp.o


# Object files for target testmysql
testmysql_OBJECTS = \
"CMakeFiles/testmysql.dir/testmysql.cpp.o"

# External object files for target testmysql
testmysql_EXTERNAL_OBJECTS =

testmysql: CMakeFiles/testmysql.dir/testmysql.cpp.o
testmysql: CMakeFiles/testmysql.dir/build.make
testmysql: CMakeFiles/testmysql.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/yhb/test/my_webServer/test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable testmysql"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/testmysql.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/testmysql.dir/build: testmysql

.PHONY : CMakeFiles/testmysql.dir/build

CMakeFiles/testmysql.dir/requires: CMakeFiles/testmysql.dir/testmysql.cpp.o.requires

.PHONY : CMakeFiles/testmysql.dir/requires

CMakeFiles/testmysql.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/testmysql.dir/cmake_clean.cmake
.PHONY : CMakeFiles/testmysql.dir/clean

CMakeFiles/testmysql.dir/depend:
	cd /home/yhb/test/my_webServer/test/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/yhb/test/my_webServer/test /home/yhb/test/my_webServer/test /home/yhb/test/my_webServer/test/build /home/yhb/test/my_webServer/test/build /home/yhb/test/my_webServer/test/build/CMakeFiles/testmysql.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/testmysql.dir/depend
