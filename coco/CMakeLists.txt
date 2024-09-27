set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
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
				emu/coco/platform/GuiLed.hpp
				emu/coco/platform/GuiRotaryKnob.hpp
				emu/coco/platform/Loop_emu.hpp
				emu/coco/platform/Newline_emu.hpp
				emu/coco/platform/glad/glad.h
				emu/coco/platform/glad/khrplatform.h
				emu/coco/platform/font/tahoma16pt8bpp.hpp
				emu/coco/platform/font/header.hpp
				emu/coco/platform/font/footer.hpp
			PRIVATE
				emu/coco/platform/debug.cpp
				emu/coco/platform/Gui.cpp
				emu/coco/platform/GuiLed.cpp
				emu/coco/platform/GuiRotaryKnob.cpp
				emu/coco/platform/Loop_emu.cpp
				emu/coco/platform/Newline_emu.cpp
				emu/coco/platform/glad/glad.c
				emu/coco/platform/font/tahoma16pt8bpp.cpp
		)

		target_link_libraries(${PROJECT_NAME}
			coco-font::coco-font
			glfw
		)
	endif()
elseif(${PLATFORM} MATCHES "^nrf52")
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cortex ${CMAKE_CURRENT_SOURCE_DIR}/nrf52 FILES
			cortex/coco/platform/Loop_Queue.hpp
			cortex/coco/platform/Loop_SysTick.hpp
			nrf52/coco/platform/Loop_RTC0.hpp
		PRIVATE
			cortex/coco/platform/Loop_Queue.cpp
			cortex/coco/platform/Loop_SysTick.cpp
			nrf52/coco/platform/Loop_RTC0.cpp
	)
elseif(${PLATFORM} MATCHES "^stm32c0")
	# STM32C0 lacks TIM2
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cortex ${CMAKE_CURRENT_SOURCE_DIR}/stm32 FILES
			cortex/coco/platform/Loop_Queue.hpp
			cortex/coco/platform/Loop_SysTick.hpp
			stm32/coco/platform/Loop_TIM.hpp
		PRIVATE
			cortex/coco/platform/Loop_Queue.cpp
			cortex/coco/platform/Loop_SysTick.cpp
			stm32/coco/platform/Loop_TIM.cpp
	)
elseif(${PLATFORM} MATCHES "^stm32")
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/cortex ${CMAKE_CURRENT_SOURCE_DIR}/stm32 FILES
			cortex/coco/platform/Loop_Queue.hpp
			cortex/coco/platform/Loop_SysTick.hpp
			stm32/coco/platform/Loop_TIM.hpp
			stm32/coco/platform/Loop_TIM2.hpp
		PRIVATE
			cortex/coco/platform/Loop_Queue.cpp
			cortex/coco/platform/Loop_SysTick.cpp
			stm32/coco/platform/Loop_TIM.cpp
			stm32/coco/platform/Loop_TIM2.cpp
	)
endif()

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
	FILE_SET platform_headers DESTINATION include # platform headers go into include/coco/platform
)
