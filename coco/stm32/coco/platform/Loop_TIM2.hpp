#pragma once

#include <coco/Callback.hpp>
#include <coco/Frequency.hpp>
#include <coco/IntrusiveList.hpp>
#include <coco/IntrusiveMpscQueue.hpp>
#include <coco/platform/Loop_Queue.hpp>
#include <coco/platform/platform.hpp>


namespace coco {

/*
 * Implementation of the Loop interface using TIM2
 *
 * Reference manual:
 *   f0:
 *     https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       TIM: Sections 17-21
 *   f334:
 *     https://www.st.com/resource/en/reference_manual/rm0364-stm32f334xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       TIM: Sections 18-20
 *   g4:
 *     https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *       TIM: Sections 28-31
 *
 * Resources:
 *   TIM2
 *     CC1
 */
class Loop_TIM2 : public Loop_Queue {
public:
    enum class Mode {
        /// continuously poll for events
        POLL,

        /// Wait for events using WFE instruction
        /// Note: Causes connection loss of the debugger on STM32G4
        WAIT,

        /// poll for events with watchdog of 0.5s
        WATCHDOG
    };

protected:
    /**
     * Internal constructor
     * @param prescaler prescaler for 1ms timer resolution from ABP1 clock for TIM2
     * @param mode loop mode
     */
    Loop_TIM2(int prescaler, Mode mode);

public:
    /**
     * Constructor
     * @param timerClock APB1 timer clock frequency (APB1_TIMER_CLOCK), maximum is 65MHz
     * @param mode loop mode
     */
    Loop_TIM2(Kilohertz<> timerClock, Mode mode = Mode::POLL) : Loop_TIM2(timerClock.value - 1, mode) {}
    ~Loop_TIM2() override;

    void run() override;
    [[nodiscard]] Time now() override;
    [[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;
    using Loop::sleep;

protected:
    Mode mode;
};

} // namespace coco
