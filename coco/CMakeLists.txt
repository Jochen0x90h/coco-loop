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
		#PUBLIC FILE_SET headers FILES
		PRIVATE
			native/loop.cpp
			native/loop.inc.hpp
	)
elseif(${PLATFORM} MATCHES "^nrf52")
	target_sources(${PROJECT_NAME}
		#PUBLIC FILE_SET headers FILES
		PRIVATE
			nrf52/loop.cpp
	)
elseif (${PLATFORM} MATCHES "^stm32f0")
	target_sources(${PROJECT_NAME}
		#PUBLIC FILE_SET headers FILES
		PRIVATE
			stm32f0/loop.cpp
	)
endif ()

#target_include_directories(${PROJECT_NAME}
#	PUBLIC	
#)

target_link_libraries(${PROJECT_NAME}
	coco::coco
)

# install the library
install(TARGETS ${PROJECT_NAME}
	FILE_SET headers DESTINATION include/coco
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
