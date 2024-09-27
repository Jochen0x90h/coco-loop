#pragma once

#include <coco/Coroutine.hpp>
#include <coco/Time.hpp>


namespace coco {

/**
 * Main event loop. Subclasses implement the event loop for different target platforms
 */
class Loop {
public:
    using Duration = Milliseconds<>;
    using Time = TimeMilliseconds<>;

    virtual ~Loop() {}

    /**
     * Run the event loop until exit() gets called
     */
    virtual void run() = 0;

    /**
     * Exit the event loop on "normal" operating systems
     */
    void exit() {this->exitFlag = true;}

    /**
     * Get current time in milliseconds
     * @return current time
     */
    [[nodiscard]] Time virtual now() = 0;

    /**
     * Suspend execution using co_await until a given time. Only up to TIMER_COUNT coroutines can wait simultaneously.
     * @param time time point
     */
    [[nodiscard]] virtual Awaitable<CoroutineTimedTask> sleep(Time time) = 0;

    /**
     * Suspend execution using co_await for a given duration. Only up to TIMER_COUNT coroutines can wait simultaneously.
     * @param duration duration
     */
    [[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Duration duration) {return sleep(now() + duration);}

    /**
     * Yield control to other coroutines. Can be used to do longer processing in a cooperative way.
     */
    [[nodiscard]] virtual Awaitable<CoroutineTimedTask> yield() {return sleep(now());}


    bool exitFlag = false;
};

} // namespace coco
