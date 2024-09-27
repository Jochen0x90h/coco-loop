#pragma once

#include <coco/platform/Loop_TIM.hpp>
#include <coco/platform/Loop_SysTick.hpp>
#include <coco/board/config.hpp>


using namespace coco;


// drivers for LoopTest
struct Drivers {
    Loop_TIM loop{timer::TIM17_INFO, APB_TIMER_CLOCK};
	//Loop_SysTick loop{AHB_CLOCK};
};

Drivers drivers;

extern "C" {
void SysTick_Handler() {
    //drivers.loop.SysTick_Handler();
}
}
