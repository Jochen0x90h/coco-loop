cmake_minimum_required(VERSION 3.19)

# set project
project(coco-loop)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(VERBOSE 1)
#set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

# configure languages
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)

# build type
message("*** Type: ${CMAKE_BUILD_TYPE}${CMAKE_CONFIGURATION_TYPES}")
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

# compiler dependent configuration
message("*** Compiler: ${CMAKE_CXX_COMPILER_ID}")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# clang
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts -Wno-user-defined-literals")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	# visual studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4455")
else()
	# gcc
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines -fconcepts -Wno-literal-suffix")
endif()
message("*** Crosscompiling: ${CMAKE_CROSSCOMPILING}")

# enable address sanitizer
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

# platform dependent configuration
message("*** Platform: ${PLATFORM}")
if(${PLATFORM} STREQUAL "native")
	set(SERIES native)
	add_compile_definitions(NATIVE)
elseif(${PLATFORM} MATCHES "^nrf52")
	set(SERIES nrf52)
	add_compile_definitions(NRF52)

	# compiler flags
	set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mabi=aapcs")
	set(FPU_FLAGS "-mfpu=fpv4-sp-d16 -mfloat-abi=hard")

	# keep every function in a separate section, this allows linker to discard unused ones
	set(C_FLAGS "-D__STACK_SIZE=8192 -D__HEAP_SIZE=8192 -fshort-enums -fno-exceptions -fdata-sections -ffunction-sections -Wall")
	#-fno-builtin
	set(CXX_FLAGS "${C_FLAGS} -fno-rtti -fno-use-cxa-atexit")

	# let linker dump unused sections, use newlib in nano version, add standard libs at end so that their symbols get found
	# https://interrupt.memfault.com/blog/how-to-write-linker-scripts-for-firmware
	# use target_link_directories to let the linker find link.ld
	set(LINKER_FLAGS "-Wl,--gc-sections -Wl,--undefined=SystemInit -specs=nano.specs -specs=nosys.specs -Tlink.ld")

	# set flags
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_FLAGS} ${FPU_FLAGS} ${C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPU_FLAGS} ${FPU_FLAGS} ${CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
elseif(${PLATFORM} MATCHES "^stm32f0")
	set(SERIES stm32f0)
	add_compile_definitions(STM32F0)

	# compiler flags
	set(CPU_FLAGS "-mcpu=cortex-m0 -mthumb -mabi=aapcs")
	set(FPU_FLAGS "-mfloat-abi=soft")

	# keep every function in a separate section, this allows linker to discard unused ones
	set(C_FLAGS "-D__STACK_SIZE=8192 -D__HEAP_SIZE=8192 -fshort-enums -fno-exceptions -fdata-sections -ffunction-sections -Wall")
	#-fno-builtin
	set(CXX_FLAGS "${C_FLAGS} -fno-rtti -fno-use-cxa-atexit")

	# let linker dump unused sections, use newlib in nano version, add standard libs at end so that their symbols get found
	# https://interrupt.memfault.com/blog/how-to-write-linker-scripts-for-firmware
	# use target_link_directories to let the linker find link.ld
	set(LINKER_FLAGS "-Wl,--gc-sections -Wl,--undefined=SystemInit -specs=nano.specs -specs=nosys.specs -Tlink.ld")

	# set flags
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_FLAGS} ${FPU_FLAGS} ${C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPU_FLAGS} ${FPU_FLAGS} ${CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
endif()
message("*** Series: ${SERIES}")


# dependencies
find_package(coco CONFIG)
find_package(coco-devboards CONFIG)
foreach(BOARD ${coco-devboards_COMPONENT_NAMES})
	message("*** Board: ${BOARD}")
endforeach()

add_subdirectory(coco)

# test executables
add_subdirectory(test)
