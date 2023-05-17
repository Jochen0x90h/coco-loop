add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET headers TYPE HEADERS BASE_DIRS FILES
		Loop.hpp
	PRIVATE
		Loop.cpp
	)

if(${PLATFORM} STREQUAL "native" OR ${PLATFORM} STREQUAL "emu")
	# native platform (Windows, MacOS, Linux)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/native FILES
			native/coco/platform/Loop_native.hpp
		#PRIVATE
	)
	if(WIN32)
		# io completion ports
		target_sources(${PROJECT_NAME}
			PUBLIC FILE_SET platform_headers FILES
				native/coco/platform/Loop_Win32.hpp
			PRIVATE
				native/coco/platform/Loop_Win32.cpp
		)
	#todo: use try_compile
	#elseif(${OS} STREQUAL "Macos" OR ${OS} STREQUAL "FreeBSD")
		# kqueue
	#else(${OS} STREQUAL "Linux")
		# epoll
	endif()

	# graphical emulator
	if(${PLATFORM} STREQUAL "emu")
		target_sources(${PROJECT_NAME}
			PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/emu FILES
				emu/coco/platform/Gui.hpp
				emu/coco/platform/Loop_emu.hpp
				emu/coco/platform/Newline_emu.hpp
				emu/coco/platform/glad/glad.h
				emu/coco/platform/glad/khrplatform.h
			PRIVATE
				emu/coco/platform/Gui.cpp
				emu/coco/platform/Loop_emu.cpp
				emu/coco/platform/Newline_emu.cpp
				emu/coco/platform/glad/glad.c
		)

		target_link_libraries(${PROJECT_NAME}
			glfw
		)
	endif()
elseif(${PLATFORM} MATCHES "^nrf52")
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/nrf52 FILES
			nrf52/coco/platform/Loop_RTC0.hpp
		PRIVATE
			nrf52/coco/platform/Loop_RTC0.cpp
	)
elseif (${PLATFORM} MATCHES "^stm32f0")
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0 FILES
			stm32f0/coco/platform/Loop_TIM2.hpp
		PRIVATE
			stm32f0/coco/platform/Loop_TIM2.cpp
	)
endif ()

target_include_directories(${PROJECT_NAME}
	PRIVATE
		../
)

target_link_libraries(${PROJECT_NAME}
	coco::coco
)

# install the library
install(TARGETS ${PROJECT_NAME}
	FILE_SET headers DESTINATION include/coco
	FILE_SET platform_headers DESTINATION include
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
