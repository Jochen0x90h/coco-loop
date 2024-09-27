# Generate a test for a board
# TEST the test application, implemented in ${TEST}.cpp
# BOARD_LIB a library for a board containing SystemInit() and a linker script for embedded platforms
function(board_test TEST BOARD_LIB)
    # check if board library exists for the current platform
    if(TARGET ${BOARD_LIB})
        string(REGEX REPLACE ".*\\:" "" BOARD ${BOARD_LIB})
        set(NAME "${TEST}-${BOARD}")
        message("*** Test ${TEST} on board ${BOARD}")

        add_executable(${NAME}
            ${TEST}.cpp
        )
        target_include_directories(${NAME}
            PRIVATE
                ../
                ${BOARD}
        )
        target_link_libraries(${NAME}
            ${BOARD_LIB}
            ${PROJECT_NAME}
        )
        #target_link_options(${NAME}
        #	PUBLIC
        #		LINKER:-Map=${NAME}.map
        #)

        # generate hex file for flashing the target
        if(${CMAKE_CROSSCOMPILING})
            #message("*** Generate Hex for ${NAME} using ${CMAKE_OBJCOPY}")
            add_custom_command(TARGET ${NAME}
                POST_BUILD
                COMMAND ${CMAKE_OBJCOPY} -O ihex ${NAME} ${NAME}.hex
            )
        endif()
    endif()
endfunction()

#message("*** coco-devboards: ${coco-devboards_COMPONENT_NAMES}")

board_test(LoopTest coco-devboards::native)
board_test(LoopTest coco-devboards::emu)
board_test(LoopTest coco-devboards::nrf52dongle)
board_test(LoopTest coco-devboards::canable02)
board_test(LoopTest coco-devboards::stm32f0discovery)
board_test(LoopTest coco-devboards::stm32f3348discovery)
board_test(LoopTest coco-devboards::stm32f401nucleo)
board_test(LoopTest coco-devboards::stm32c031nucleo)
board_test(LoopTest coco-devboards::stm32g431nucleo)
board_test(LoopTest coco-devboards::stm32g474nucleo)
