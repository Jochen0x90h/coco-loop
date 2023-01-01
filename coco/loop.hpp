#pragma once

#include <coco/Coroutine.hpp>
#include <coco/Time.hpp>


namespace coco {
namespace loop {

/**
 * Initialize the event loop
 */
void init();

/**
 * Run the event loop
 */
void run();

/**
 * Get current time in milliseconds
 * @return current time
 */
Time now();

/**
 * Suspend execution using co_await until a given time. Only up to TIMER_COUNT coroutines can wait simultaneously.
 * @param time time point
 */
[[nodiscard]] Awaitable<Time> sleep(Time time);

/**
 * Suspend execution using co_await for a given duration. Only up to TIMER_COUNT coroutines can wait simultaneously.
 * @param duration duration
 */
[[nodiscard]] inline Awaitable<Time> sleep(Duration duration) {return sleep(now() + duration);}


// busy waiting, only for debug purposes, not very precise
void sleepBlocking(int us);

} // namespace loop
} // namespace coco
