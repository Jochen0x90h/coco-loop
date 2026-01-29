#include "Loop_SysTick.hpp"


namespace coco {

Loop_SysTick::Loop_SysTick(int khz, Mode mode)
    : khz_(khz)
#ifndef NRF52
    , wait_(mode == Mode::WAIT)
#endif
{
    // timer interval in milliseconds
    interval_ = endTime_ = 0x1000000 / khz;

    // set reload value so that timeout occurs at whole millisecond boundaries
    SysTick->LOAD = khz * interval_ - 1;

    // reset counter and timeout flag
    SysTick->VAL = 0;

    SysTick->CTRL =
#ifdef STM32
        0 // STM32: Divides HCLK by 8
#else
        SysTick_CTRL_CLKSOURCE_Msk
#endif
        | (mode >= Mode::INTERRUPT ? SysTick_CTRL_TICKINT_Msk : 0) // interrupt mode
        | SysTick_CTRL_ENABLE_Msk;
}

Loop_SysTick::~Loop_SysTick() {
    SysTick->CTRL = 0;
}

void Loop_SysTick::run() {
    while (!exitFlag_) {
        // wait for event if sleep time has not yet passed
        // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
#ifndef NRF52
        if (wait_) {
            // time when the current SysTick interval ends
            Time endTime = Time(endTime_);

            // get sleep time
            Time sleepTime = sleepTasks2_.getFirstTime(sleepTasks1_.getFirstTime(endTime));

            // check if we can seep until the end of the current interval
            if (sleepTime == endTime) {
                // data synchronization barrier
                __DSB();

                // wait for event (push() and SysTick_Handler() send an event using __SEV())
                __WFE();
            }
        }
#endif

        // call all handlers
        Handler *handler;
        while ((handler = handlerQueue_.pop()) != nullptr) {
            handler->handle();
        }

        // resume coroutines waiting on sleep()
        auto currentTime = now();
        sleepTasks1_.doUntil(currentTime);
        sleepTasks2_.doUntil(currentTime);
    }
    exitFlag_ = false;
}

Loop::Time Loop_SysTick::now() {
    uint32_t counter = SysTick->VAL;

    // check for count flag when in interrupt-less mode
    if ((SysTick->CTRL & (SysTick_CTRL_COUNTFLAG_Msk | SysTick_CTRL_TICKINT_Msk)) == SysTick_CTRL_COUNTFLAG_Msk) {
        // reload counter in case overflow happened after reading the counter
        counter = SysTick->VAL;

        // advance base time
        endTime_ = endTime_ + interval_;
    }

    return Time(endTime_ - (counter == 0 ? interval_ : counter / khz_));
}

Awaitable<CoroutineTimedTask> Loop_SysTick::sleep(Time time) {
    return {sleepTasks2_, time};
}

} // namespace coco
