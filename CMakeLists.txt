cmake_minimum_required(VERSION 3.19)

# set project
project(coco-loop)

# configure languages
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)

# build type
message("*** Type: ${CMAKE_BUILD_TYPE}${CMAKE_CONFIGURATION_TYPES}")
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

# compiler
message("*** Compiler: ${CMAKE_CXX_COMPILER_ID}")
message("*** Crosscompiling: ${CMAKE_CROSSCOMPILING}")

# enable address sanitizer
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

# platform
#message("*** OS: ${OS}")
message("*** Platform: ${PLATFORM}")

# dependencies
if(${PLATFORM} STREQUAL "emu")
	find_package(glfw3 CONFIG)
endif()
find_package(coco CONFIG)
find_package(coco-devboards CONFIG)
foreach(BOARD ${coco-devboards_COMPONENT_NAMES})
	message("*** Board: ${BOARD}")
endforeach()

add_subdirectory(coco)

# test executables
add_subdirectory(test)
