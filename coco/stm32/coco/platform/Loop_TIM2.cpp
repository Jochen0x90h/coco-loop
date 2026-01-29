#include "Loop_TIM2.hpp"
//#include <coco/debug.hpp>
#include <coco/platform/nvic.hpp>
#include <coco/platform/timer.hpp>


#ifdef TIM2

namespace coco {

// maximum sleep time is 24.8 days
constexpr int MAX_SLEEP = 0x7fffffff;


Loop_TIM2::Loop_TIM2(int prescaler, Mode mode) : mode_(mode) {
    // disabled interrupts trigger an event and wake up the processor from WFE
    // stm32f0: see chapter 5.3.3 in reference manual (https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
    if (mode == Mode::WAIT)
        SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

    // configure and start clock
    timer::TIM2_INFO.enableClock()
        .setPrescaler(prescaler)
        .set(timer::Interrupt::COMPARE1, timer::DmaRequest::NONE)
        .start();
}

Loop_TIM2::~Loop_TIM2() {
}

void Loop_TIM2::run() {
    // enable watchdog
    if (mode_ == Mode::WATCHDOG)
        IWDG->KR = 0xCCCC;

    // set watchdog registers
    //IWDG->KR = 0x5555;
    //while (IWDG->SR != 0);

    Time currentTime = now();
    while (!exitFlag_) {
        // restart watchdog
        IWDG->KR = 0xAAAA;

        // wait for event if sleep time has not yet passed
        // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
        if (mode_ == Mode::WAIT) {
            // get sleep time
            Time sleepTime = sleepTasks2_.getFirstTime(sleepTasks1_.getFirstTime(currentTime + MAX_SLEEP * 1ms));

            // set new timeout and clear pending interrupt flags at peripheral and NVIC
            TIM2->CCR1 = sleepTime.value;
            TIM2->SR = ~TIM_SR_CC1IF;
            nvic::clear(TIM2_IRQn);

            // wait if timeout has not passed yet
            bool notPassed = sleepTime.value - int(TIM2->CNT) > 0;
            if (notPassed) {
                //debug::toggleGreen();

                // data synchronization barrier
                __DSB();

                // wait for event (interrupts trigger an event due to SEVONPEND)
                __WFE();
            }
        }

        // call all handlers
        Handler *handler;
        while ((handler = handlerQueue_.pop()) != nullptr) {
            handler->handle();
        }

        // resume coroutines waiting on sleep()
        currentTime = now();
        sleepTasks1_.doUntil(currentTime);
        sleepTasks2_.doUntil(currentTime);
    }
    exitFlag_ = false;
}

Loop::Time Loop_TIM2::now() {
    return Time(TIM2->CNT);
}

Awaitable<CoroutineTimedTask> Loop_TIM2::sleep(Time time) {
    return {sleepTasks2_, time};
}

} // namespace coco

#endif // TIM2
