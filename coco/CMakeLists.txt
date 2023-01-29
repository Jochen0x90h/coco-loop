add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET headers TYPE HEADERS BASE_DIRS FILES
		Loop.hpp
	PRIVATE
		Loop.cpp
	)

if(${PLATFORM} STREQUAL "native")
	# native platform (Windows, MacOS, Linux)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/native FILES
			native/coco/platform/Handler.hpp
			native/coco/platform/Loop_native.hpp
		PRIVATE
			native/coco/platform/Handler.cpp
	)
	if(${OS} STREQUAL "Windows")
		# io completion ports
		target_sources(${PROJECT_NAME}
			PUBLIC FILE_SET platform_headers FILES
				native/coco/platform/Loop_Win32.hpp
			PRIVATE
				native/coco/platform/Loop_Win32.cpp
		)
	elseif(${OS} STREQUAL "Macos" OR ${OS} STREQUAL "FreeBSD")
		# kqueue
	else(${OS} STREQUAL "Linux")
		# epoll
	endif()
elseif(${PLATFORM} MATCHES "^nrf52")
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/nrf52 FILES
			nrf52/coco/platform/Handler.hpp
			nrf52/coco/platform/Loop_RTC0.hpp
		PRIVATE
			nrf52/coco/platform/Handler.cpp
			nrf52/coco/platform/Loop_RTC0.cpp
	)
elseif (${PLATFORM} MATCHES "^stm32f0")
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0 FILES
			stm32f0/coco/platform/Handler.hpp
			stm32f0/coco/platform/Loop_TIM2.hpp
		PRIVATE
			stm32f0/coco/platform/Handler.cpp
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
