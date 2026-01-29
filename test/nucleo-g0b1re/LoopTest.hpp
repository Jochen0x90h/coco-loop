#pragma once

#include <coco/platform/Loop_SysTick.hpp>
#include <coco/board/config.hpp>


using namespace coco;


// drivers for LoopTest
struct Drivers {
	Loop_SysTick loop{AHB_CLOCK, Loop_SysTick::Mode::POLL};
};

Drivers drivers;
