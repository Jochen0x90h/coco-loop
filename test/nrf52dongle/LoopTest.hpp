#pragma once

#include <coco/platform/Loop_RTC0.hpp>
#include <coco/platform/Loop_SysTick.hpp>
#include <coco/board/config.hpp>


using namespace coco;


// drivers for LoopTest
struct Drivers {
	Loop_RTC0 loop{Loop_RTC0::Mode::WAIT};
	//Loop_SysTick loop{Loop_SysTick::Mode::POLL};
};

Drivers drivers;

extern "C" {
void SysTick_Handler() {
    //drivers.loop.SysTick_Handler();
}
}
