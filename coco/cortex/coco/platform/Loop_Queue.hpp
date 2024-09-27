#pragma once

#include <coco/Loop.hpp>
#include <coco/Callback.hpp>
#include <coco/IntrusiveMpscQueue.hpp>
#include <coco/platform/platform.hpp>


namespace coco {

/**
 * Base class for event loop on ARM Cortex
 *
 * These implementations are available:
 *   SysTick without interrupt: No calculation should take longer than 1/5 second not to miss ticks, no WFE
 *   SysTick with interrupt: Generates an interrupt every millisecond, supports WFE depending on platform
 *   NRF52 using RTC0: Uses the 24 bit timer RTC0, supports WFE
 *   STM32 using TIM2: Uses the 32 bit timer TIM2, supports WFE
 *   STM32 using TIMx: Uses a 16 bit general purpose timer, supports WFE
 */
class Loop_Queue : public Loop {
public:

    void invoke(TimedTask<Callback> &task, Time time) {
        task.cancelAndSet(time);
        this->sleepTasks1.add(task);
    }

    void invoke(TimedTask<Callback> &task, Duration duration) {
        task.cancelAndSet(now() + duration);
        this->sleepTasks1.add(task);
    }

    void invoke(TimedTask<Callback> &task) {
        task.cancelAndSet(now());
        this->sleepTasks1.add(task);
    }

    /**
     * Event handler that handles finished device operations
     */
    class Handler : public IntrusiveMpscQueueNode {
    public:
        virtual ~Handler() {}
        virtual void handle() = 0;
    };

    /**
     * Push a handler for a finished device operation onto the handler queue so that the main application gets
     * notified. Can be called from the interrupt service routine of the device e.g. when a read or write operation
     * has finished.
     */
    void push(Handler &handler) {
        this->handlerQueue.push(handler);

        // set event flag so that the next __WFE() does not sleep as there are new elements in the handler queue
        __SEV();
    }

protected:

    // sleep tasks
    TimedTaskList<Callback> sleepTasks1;
    CoroutineTimedTaskList sleepTasks2;

    // handlers for finished device operations
    IntrusiveMpscQueue<Handler> handlerQueue;
};

} // namespace coco
