#pragma once

#include <coco/Loop.hpp>
#include <coco/Callback.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef interface
#undef INTERFACE
#undef IN
#undef OUT
#undef READ_ATTRIBUTES
#undef ERROR


namespace coco {

/// @brief Implementation of the Loop interface using Win32 and io completion ports
///
class Loop_Win32 : public Loop {
public:

    Loop_Win32();
    ~Loop_Win32() override;

    // Loop methods
    void run() override;
    [[nodiscard]] Time now() override;
    [[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;
    using Loop::sleep;


    void invoke(TimedTask<Callback> &task, Time time) {
        task.cancelAndSet(time);
        this->sleepTasks1_.add(task);
    }

    void invoke(TimedTask<Callback> &task, Duration duration) {
        task.cancelAndSet(now() + duration);
        this->sleepTasks1_.add(task);
    }

    void invoke(TimedTask<Callback> &task) {
        task.cancelAndSet(now());
        this->sleepTasks1_.add(task);
    }


    /// @brief IO Completion handler.
    ///
    class CompletionHandler {
    public:
        virtual ~CompletionHandler() {}
        virtual void handle(OVERLAPPED *overlapped) = 0;
    };

    /// @brief Handle events and wait at most the given number of milliseconds for new events
    /// @param wait maximum time to wait in milliseconds
    bool handleEvents(int wait = std::numeric_limits<int>::max() / 2);

    // io completion port
    HANDLE port;

protected:
    // frequency for QueryPerformanceCounter
    int64_t frequency_;

    // sleep tasks
    TimedTaskList<Callback> sleepTasks1_;
    CoroutineTimedTaskList sleepTasks2_;
};

} // namespace coco
