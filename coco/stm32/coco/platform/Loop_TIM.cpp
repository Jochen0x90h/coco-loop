#include "Loop_TIM.hpp"
//#include <coco/debug.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

// maximum sleep time is 32.767 seconds
constexpr int MAX_SLEEP = 0x7fff;


Loop_TIM::Loop_TIM(const TimerInfo &timerInfo, int prescaler, Mode mode)
    : timer_(timerInfo.timer), timerIrq_(timerInfo.irq<timer::Irq::CC>()), mode_(mode)
{
    // disabled interrupts trigger an event and wake up the processor from WFE
    // stm32f0: see chapter 5.3.3 in reference manual (https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
    if (mode == Mode::WAIT)
        SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

    // configure and start clock
    timerInfo.enableClock()
        .setReload(0xffff) // treat as 16 bit timer even if it is 32 bit
        .setPrescaler(prescaler)
        .set(timer::Interrupt::COMPARE1, timer::DmaRequest::NONE)
        .start();
}

Loop_TIM::~Loop_TIM() {
}

void Loop_TIM::run() {
    // enable watchdog
    if (mode_ == Mode::WATCHDOG)
        IWDG->KR = 0xCCCC;

    // set watchdog registers
    //IWDG->KR = 0x5555;
    //while (IWDG->SR != 0);

    Time currentTime = now();
    while (!exitFlag_) {
        auto timer = timer_;

        // restart watchdog
        IWDG->KR = 0xAAAA;

        // wait for event if sleep time has not yet passed
        // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
        if (mode_ == Mode::WAIT) {
            // get sleep time (point in time when the first task is due)
            Time sleepTime = sleepTasks2_.getFirstTime(sleepTasks1_.getFirstTime(currentTime + MAX_SLEEP * 1ms));

            // set new timeout and clear pending interrupt flags at peripheral and NVIC
            int32_t timeout = sleepTime.value & 0xffff; // lower 16 bit are relevant
            timer->CCR1 = timeout;
            timer->SR = ~TIM_SR_CC1IF;
            nvic::clear(timerIrq_);

            // wait if timeout has not passed yet
            bool notPassed = ((timeout - int32_t(timer->CNT)) << 16) > 0;
            if (notPassed) {
                //debug::toggleGreen();

                // data synchronization barrier
                __DSB();

                // wait for event (interrupts trigger an event due to SEVONPEND)
                __WFE();
            }
        }

        // call all handlers
        CompletionHandler *handler;
        while ((handler = handlerQueue_.pop()) != nullptr) {
            handler->onCompletion();
        }

        // resume coroutines waiting on sleep()
        currentTime = now();
        sleepTasks1_.doUntil(currentTime, [](TimeoutHandler &handler) {handler.onTimeout();});
        sleepTasks2_.doUntil(currentTime);
    }
    exitFlag_ = false;
}

Loop::Time Loop_TIM::now() {
    //return Time(TIM2->CNT);
    auto &timer = timer_;

    uint32_t counter = timer->CNT;
    if (timer->SR & TIM_SR_UIF) {
        timer->SR = ~TIM_SR_UIF;

        // reload counter in case overflow happened after reading the counter
        counter = timer->CNT;

        // advance base time by 65536
        baseTime_ += 0x10000;
    }
    return Time(baseTime_ + counter);
}

Awaitable<CoroutineTimedTask> Loop_TIM::sleep(Time time) {
    return {sleepTasks2_, time};
}

} // namespace coco
