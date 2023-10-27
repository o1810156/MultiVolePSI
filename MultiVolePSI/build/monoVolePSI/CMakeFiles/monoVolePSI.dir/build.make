# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /MultiVolePSI

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /MultiVolePSI/build

# Include any dependencies generated for this target.
include monoVolePSI/CMakeFiles/monoVolePSI.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include monoVolePSI/CMakeFiles/monoVolePSI.dir/compiler_depend.make

# Include the progress variables for this target.
include monoVolePSI/CMakeFiles/monoVolePSI.dir/progress.make

# Include the compile flags for this target's objects.
include monoVolePSI/CMakeFiles/monoVolePSI.dir/flags.make

monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o: monoVolePSI/CMakeFiles/monoVolePSI.dir/flags.make
monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o: /MultiVolePSI/monoVolePSI/mono_psi.cpp
monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o: monoVolePSI/CMakeFiles/monoVolePSI.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/MultiVolePSI/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o"
	cd /MultiVolePSI/build/monoVolePSI && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o -MF CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o.d -o CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o -c /MultiVolePSI/monoVolePSI/mono_psi.cpp

monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/monoVolePSI.dir/mono_psi.cpp.i"
	cd /MultiVolePSI/build/monoVolePSI && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /MultiVolePSI/monoVolePSI/mono_psi.cpp > CMakeFiles/monoVolePSI.dir/mono_psi.cpp.i

monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/monoVolePSI.dir/mono_psi.cpp.s"
	cd /MultiVolePSI/build/monoVolePSI && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /MultiVolePSI/monoVolePSI/mono_psi.cpp -o CMakeFiles/monoVolePSI.dir/mono_psi.cpp.s

# Object files for target monoVolePSI
monoVolePSI_OBJECTS = \
"CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o"

# External object files for target monoVolePSI
monoVolePSI_EXTERNAL_OBJECTS =

/MultiVolePSI/lib/libmonoVolePSI.a: monoVolePSI/CMakeFiles/monoVolePSI.dir/mono_psi.cpp.o
/MultiVolePSI/lib/libmonoVolePSI.a: monoVolePSI/CMakeFiles/monoVolePSI.dir/build.make
/MultiVolePSI/lib/libmonoVolePSI.a: monoVolePSI/CMakeFiles/monoVolePSI.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/MultiVolePSI/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library /MultiVolePSI/lib/libmonoVolePSI.a"
	cd /MultiVolePSI/build/monoVolePSI && $(CMAKE_COMMAND) -P CMakeFiles/monoVolePSI.dir/cmake_clean_target.cmake
	cd /MultiVolePSI/build/monoVolePSI && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/monoVolePSI.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
monoVolePSI/CMakeFiles/monoVolePSI.dir/build: /MultiVolePSI/lib/libmonoVolePSI.a
.PHONY : monoVolePSI/CMakeFiles/monoVolePSI.dir/build

monoVolePSI/CMakeFiles/monoVolePSI.dir/clean:
	cd /MultiVolePSI/build/monoVolePSI && $(CMAKE_COMMAND) -P CMakeFiles/monoVolePSI.dir/cmake_clean.cmake
.PHONY : monoVolePSI/CMakeFiles/monoVolePSI.dir/clean

monoVolePSI/CMakeFiles/monoVolePSI.dir/depend:
	cd /MultiVolePSI/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /MultiVolePSI /MultiVolePSI/monoVolePSI /MultiVolePSI/build /MultiVolePSI/build/monoVolePSI /MultiVolePSI/build/monoVolePSI/CMakeFiles/monoVolePSI.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : monoVolePSI/CMakeFiles/monoVolePSI.dir/depend

