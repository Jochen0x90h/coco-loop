#pragma once

#include "Loop_Queue.hpp"
#include <coco/enum.hpp>
#include <coco/Frequency.hpp>
#include <atomic>


namespace coco {

/// @brief Implementation of the Loop interface using SysTick
///
/// Reference manual:
///   https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-timer--systick
class Loop_SysTick : public Loop_Queue {
public:
    enum class Mode {

        /// @brief Poll the SysTick without interrupt.
        /// Note that the event loop should run at least 5 times per second (platform dependent) to prevent missing
        /// counter rollovers, therefore use yield() in longer calculations
        POLL,

        /// @brief Use interrupts to prevent missing counter rollovers.
        /// Add SystTick interrupt handler to your drivers:
        /// extern "C" {
        /// void SysTick_Handler() {
        ///   drivers.loop.SysTick_Handler();
        /// }}
        INTERRUPT,

#ifndef NRF52
        /// @brief Use interrupts and __WFE() instruction to sleep when nothing to do during the current interval.
        /// Add SystTick interrupt handler to your drivers, see INTERRUPT.
        /// Not supported for NRF52 as SysTick stops on WFE.
        WAIT
#endif
    };

protected:
    /// @brief Internal constructor
    /// @param khz SysTick clock frequency in kilohertz
    /// @param mode Mode
    Loop_SysTick(int khz, Mode mode);

public:

    /// @brief Constructor
    /// @param sysClock system clock frequency
    /// @param mode Mode
    Loop_SysTick(Kilohertz<> sysClock, Mode mode = Mode::POLL)
#ifdef STM32 // Divides HCLK by 8
        : Loop_SysTick(sysClock.value >> 3, mode) {}
#else
        : Loop_SysTick(sysClock.value, mode) {}
#endif

#ifdef NRF52
    /// @brief Constructor
    /// @param mode Mode
    /// For NRF52, the system clock is fixed at 64MHz
    Loop_SysTick(Mode mode = Mode::POLL) : Loop_SysTick(64000, mode) {}
#endif

    ~Loop_SysTick() override;

    void run() override;
    [[nodiscard]] Time now() override;
    [[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;
    using Loop::sleep;

    /// @brief Call from SysTick_Handler interrupt when Mode::INTERRUPT or Mode::WAIT is used
    ///
    void SysTick_Handler() {
        endTime_ = endTime_ + interval_;

        // set event flag so that the next __WFE() does not sleep as time has incremented
        __SEV();
    }

protected:
    uint32_t khz_;
    bool wait_;

    // duration of counting interval in milliseconds
    uint16_t interval_;

    // end time of the current SysTick interval
    std::atomic<uint32_t> endTime_;
};

} // namespace coco
