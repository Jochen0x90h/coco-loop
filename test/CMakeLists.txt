# hex generator
separate_arguments(GENERATE_HEX)
macro(generate_hex TARGET)
	if(NOT "${GENERATE_HEX}" STREQUAL "" AND NOT "${GENERATE_HEX}" STREQUAL "None")
		#message("*** Generate Hex for ${TARGET}: ${GENERATE_HEX}")
		add_custom_command(TARGET ${TARGET}
			POST_BUILD
			COMMAND ${GENERATE_HEX} ${TARGET} ${TARGET}.hex
		)
	endif()
endmacro()


# iterate over boards and generate tests
foreach(COMPONENT ${coco-devboards_COMPONENT_NAMES})
	string(REGEX REPLACE ".*\\:" "" BOARD ${COMPONENT})
	message("*** Board: ${BOARD}")

	# check if board specific header exists
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}/LoopTest.hpp)
		set(NAME "LoopTest-${BOARD}")
		add_executable(${NAME}
			LoopTest.cpp
		)
		target_include_directories(${NAME}
			PRIVATE
				../
				${BOARD}
		)

		# find link.ld of board
		get_target_property(INCLUDES ${COMPONENT} INTERFACE_INCLUDE_DIRECTORIES)
		target_link_directories(${NAME}
			PRIVATE
				${INCLUDES}
		)

		target_link_libraries(${NAME}
			coco::coco
			${COMPONENT}
			${PROJECT_NAME}
		)

		# windows specific libraries
		if(WIN32)
			target_link_libraries(${NAME} winmm)
		endif()

		# generate hex file for flashing the target
		generate_hex(${NAME})
	endif()
endforeach()
