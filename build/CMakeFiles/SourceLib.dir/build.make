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
CMAKE_SOURCE_DIR = /home/yhb/test/my_webServer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/yhb/test/my_webServer/build

# Include any dependencies generated for this target.
include CMakeFiles/SourceLib.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/SourceLib.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/SourceLib.dir/flags.make

CMakeFiles/SourceLib.dir/main.cpp.o: CMakeFiles/SourceLib.dir/flags.make
CMakeFiles/SourceLib.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/yhb/test/my_webServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/SourceLib.dir/main.cpp.o"
	/usr/bin/g++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/SourceLib.dir/main.cpp.o -c /home/yhb/test/my_webServer/main.cpp

CMakeFiles/SourceLib.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/SourceLib.dir/main.cpp.i"
	/usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/yhb/test/my_webServer/main.cpp > CMakeFiles/SourceLib.dir/main.cpp.i

CMakeFiles/SourceLib.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/SourceLib.dir/main.cpp.s"
	/usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/yhb/test/my_webServer/main.cpp -o CMakeFiles/SourceLib.dir/main.cpp.s

CMakeFiles/SourceLib.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/SourceLib.dir/main.cpp.o.requires

CMakeFiles/SourceLib.dir/main.cpp.o.provides: CMakeFiles/SourceLib.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/SourceLib.dir/build.make CMakeFiles/SourceLib.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/SourceLib.dir/main.cpp.o.provides

CMakeFiles/SourceLib.dir/main.cpp.o.provides.build: CMakeFiles/SourceLib.dir/main.cpp.o


# Object files for target SourceLib
SourceLib_OBJECTS = \
"CMakeFiles/SourceLib.dir/main.cpp.o"

# External object files for target SourceLib
SourceLib_EXTERNAL_OBJECTS =

libSourceLib.a: CMakeFiles/SourceLib.dir/main.cpp.o
libSourceLib.a: CMakeFiles/SourceLib.dir/build.make
libSourceLib.a: CMakeFiles/SourceLib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/yhb/test/my_webServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libSourceLib.a"
	$(CMAKE_COMMAND) -P CMakeFiles/SourceLib.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/SourceLib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/SourceLib.dir/build: libSourceLib.a

.PHONY : CMakeFiles/SourceLib.dir/build

CMakeFiles/SourceLib.dir/requires: CMakeFiles/SourceLib.dir/main.cpp.o.requires

.PHONY : CMakeFiles/SourceLib.dir/requires

CMakeFiles/SourceLib.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/SourceLib.dir/cmake_clean.cmake
.PHONY : CMakeFiles/SourceLib.dir/clean

CMakeFiles/SourceLib.dir/depend:
	cd /home/yhb/test/my_webServer/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/yhb/test/my_webServer /home/yhb/test/my_webServer /home/yhb/test/my_webServer/build /home/yhb/test/my_webServer/build /home/yhb/test/my_webServer/build/CMakeFiles/SourceLib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/SourceLib.dir/depend

