#pragma once

#include "Loop_Queue.hpp"
#include <coco/enum.hpp>
#include <coco/Frequency.hpp>
#include <atomic>


namespace coco {

/**
 * Implementation of the Loop interface using SysTick
 *
 * Reference manual:
 *   https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-timer--systick
 */
class Loop_SysTick : public Loop_Queue {
public:
    enum class Mode {
        /**
         * Poll the SysTick without interrupt. Note that the event loop should run at least 5 times per second
         * (platform dependent) to prevent missing counter rollovers, therefore use yield() in longer calculations
         */
        POLL,

        /**
         * Use interrupts to prevent missing counter rollovers.
         * Add SystTick interrupt handler to your drivers:
         * extern "C" {
         * void SysTick_Handler() {
         *     drivers.loop.SysTick_Handler();
         * }}
         */
        INTERRUPT,

#ifndef NRF52
        /**
         * Use interrupts and __WFE() instruction to sleep when nothing to do during the current interval.
         * Not supported for NRF52 as SysTick stops on WFE.
         */
        WAIT
#endif
    };

protected:
    /**
     * Internal constructor
     * @param khz SysTick clock frequency in kilohertz
     * @param mode mode
     */
    Loop_SysTick(int khz, Mode mode);

public:

    /**
     * Constructor
     * @param sysClock system timer clock frequency
     * @param mode mode
     */
    Loop_SysTick(Kilohertz<> sysClock, Mode mode = Mode::POLL)
#ifdef STM32
        : Loop_SysTick(sysClock.value >> 3, mode) {}
#else
        : Loop_SysTick(sysClock.value, mode) {}
#endif

#ifdef NRF52
    /**
     * Constructor
     * @param mode mode
     */
    Loop_SysTick(Mode mode = Mode::POLL) : Loop_SysTick(64000, mode) {}
#endif

    ~Loop_SysTick() override;

    void run() override;
    [[nodiscard]] Time now() override;
    [[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;
    using Loop::sleep;

    /**
     * Call from SysTick_Handler interrupt when Mode::INTERRUPT or Mode::WAIT is used
     */
    void SysTick_Handler() {
        this->endTime = this->endTime + this->interval;

        // set event flag so that the next __WFE() does not sleep as time has incremented
        __SEV();
    }

protected:
    uint32_t khz;
    bool wait;

    // duration of one SysTick interval in milliseconds
    uint16_t interval;

    // end time of the current SysTick interval
    std::atomic<uint32_t> endTime;

    // sleep tasks
    TimedTaskList<Callback> sleepTasks1;
    CoroutineTimedTaskList sleepTasks2;

    // handlers for finished device operations
    IntrusiveMpscQueue<Handler> handlerQueue;
};

} // namespace coco
