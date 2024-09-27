#pragma once

#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/Loop_SysTick.hpp>
#include <coco/board/config.hpp>


using namespace coco;


// drivers for LoopTest
struct Drivers {
	//Loop_TIM2 loop{APB1_TIMER_CLOCK, Loop_TIM2::Mode::POLL};
	Loop_SysTick loop{AHB_CLOCK, Loop_SysTick::Mode::WAIT};
};

Drivers drivers;

extern "C" {
void SysTick_Handler() {
    drivers.loop.SysTick_Handler();
}
}
