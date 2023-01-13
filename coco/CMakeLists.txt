add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET headers TYPE HEADERS BASE_DIRS FILES
		loop.hpp
	PRIVATE
		loop.cpp
	)

if(${PLATFORM} STREQUAL "native")
	# native platform (Windows, MacOS, Linux)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/native FILES
			native/coco/platform/loop.inc.hpp
		PRIVATE
			native/coco/platform/loop.cpp
	)
elseif(${PLATFORM} MATCHES "^nrf52")
	target_sources(${PROJECT_NAME}
		#PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/nrf52 FILES
		PRIVATE
			nrf52/coco/platform/loop_RTC0.cpp
	)
elseif (${PLATFORM} MATCHES "^stm32f0")
	target_sources(${PROJECT_NAME}
		#PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0 FILES
		PRIVATE
			stm32f0/coco/platform/loop_TIM2.cpp
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
