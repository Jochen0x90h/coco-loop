#include "Loop_SysTick.hpp"


namespace coco {

Loop_SysTick::Loop_SysTick(int khz, Mode mode) : khz(khz), wait(mode > Mode::INTERRUPT) {
    this->interval = this->endTime = 0x1000000 / khz;

    SysTick->LOAD = khz * this->interval - 1;
    SysTick->VAL = 0; // reset SysTick counter value and flag

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
    while (!this->exitFlag) {
        // wait for event if sleep time has not yet passed
        // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
#ifndef NRF52
        if (this->wait) {
            // time when the current SysTick interval ends
            Time endTime = Time(this->endTime);

            // get sleep time
            Time sleepTime = this->sleepTasks2.getFirstTime(this->sleepTasks1.getFirstTime(endTime));

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
        while ((handler = this->handlerQueue.pop()) != nullptr) {
            handler->handle();
        }

        // resume coroutines waiting on sleep()
        auto currentTime = now();
        this->sleepTasks1.doUntil(currentTime);
        this->sleepTasks2.doUntil(currentTime);
    }
    this->exitFlag = false;
}

Loop::Time Loop_SysTick::now() {
    uint32_t counter = SysTick->VAL;

    // check for count flag when in interrupt-less mode
    if ((SysTick->CTRL & (SysTick_CTRL_COUNTFLAG_Msk | SysTick_CTRL_TICKINT_Msk)) == SysTick_CTRL_COUNTFLAG_Msk) {
        // reload counter in case overflow happened after reading the counter
        counter = SysTick->VAL;

        // advance base time
        this->endTime = this->endTime + this->interval;
    }

    return Time(this->endTime - (counter == 0 ? this->interval : counter / this->khz));
}

Awaitable<CoroutineTimedTask> Loop_SysTick::sleep(Time time) {
    return {this->sleepTasks2, time};
}

} // namespace coco
