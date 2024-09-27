#pragma once

#include <coco/platform/Loop_TIM2.hpp>
#include <coco/board/config.hpp>


using namespace coco;


// drivers for LoopTest
struct Drivers {
	Loop_TIM2 loop{APB1_TIMER_CLOCK, Loop_TIM2::Mode::POLL};
};

Drivers drivers;
