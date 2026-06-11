#pragma once

#include <coco/Loop.hpp>
#include <coco/Callback.hpp>
#include <coco/IntrusiveMpscQueue.hpp>
#include <coco/IntrusiveSortedTaskList.hpp>
#include <coco/platform/platform.hpp>


namespace coco {

/// @brief Base class for event loop on ARM Cortex
///
/// These implementations are available:
///   SysTick without interrupt: No calculation should take longer than 1/5 second not to miss ticks, no WFE
///   SysTick with interrupt: Generates an interrupt every millisecond, supports WFE depending on platform
///   NRF52 using RTC0: Uses the 24 bit timer RTC0, supports WFE
///   STM32 using TIM2: Uses the 32 bit timer TIM2, supports WFE
///   STM32 using TIMx: Uses a 16 bit general purpose timer, supports WFE
class Loop_Queue : public Loop {
public:

    /*void invoke(TimedTask<Callback<>> &task, Time time) {
        task.cancelAndSet(time);
        sleepTasks1_.add(task);
    }

    void invoke(TimedTask<Callback<>> &task, Duration duration = {}) {
        task.cancelAndSet(now() + duration);
        sleepTasks1_.add(task);
    }*/

    /// @brief Timeout handler.
    ///
    class TimeoutHandler : private IntrusiveListNode {
        friend class IntrusiveSortedTaskList<TimeoutHandler>;
    public:
        using IntrusiveListNode::remove;

        virtual ~TimeoutHandler() {}
        virtual void onTimeout() = 0;
        void operator ()() {onTimeout();}

    private:
        Time value;
    };

    void invoke(TimeoutHandler &handler, Time time) {
        this->sleepTasks1_.add(handler, time);
    }

    void invoke(TimeoutHandler &handler, Duration duration = {}) {
        this->sleepTasks1_.add(handler, now() + duration);
    }


    /// @brief Completion handler that handles finished device operations.
    ///
    class CompletionHandler : public IntrusiveMpscQueueNode {
    public:
        virtual ~CompletionHandler() {}
        virtual void onCompletion() = 0;
    };

    /// @brief Push a handler onto the handler queue so that the main application gets notified.
    /// Useful for example for finished device operations. Can be called from the interrupt service routine of the
    /// device e.g. when a read or write operation has finished.
    void push(CompletionHandler &handler) {
        handlerQueue_.push(handler);

        // set event flag so that the next __WFE() does not sleep as there are new elements in the handler queue
        __SEV();
    }

protected:

    // sleep tasks
    //TimedTaskList<Callback<>> sleepTasks1_;
    IntrusiveSortedTaskList<TimeoutHandler> sleepTasks1_;

    // tasks for sleep() and yield()
    CoroutineTimedTaskList sleepTasks2_;

    // handlers for finished device operations
    IntrusiveMpscQueue<CompletionHandler> handlerQueue_;
};

} // namespace coco
