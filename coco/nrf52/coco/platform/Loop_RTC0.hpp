#pragma once

#include <coco/Callback.hpp>
#include <coco/IntrusiveList.hpp>
#include <coco/IntrusiveMpscQueue.hpp>
#include <coco/platform/Loop_Queue.hpp>
#include <coco/platform/platform.hpp>


namespace coco {

/**
 * Implementation of the Loop interface using RTC0
 *
 * Reference manual:
 *   https://infocenter.nordicsemi.com/topic/ps_nrf52840/rtc.html?cp=5_0_0_5_21
 *
 * Resources:
 *   NRF_RTC0
 *     CC[0]
 */
class Loop_RTC0 : public Loop_Queue {
public:
	enum class Mode {
		/// continuously poll for events
		POLL,

		/// wait for events using WFE instruction
		WAIT,
	};

	/**
	 * Constructor
	 * @param wait wait for events
	 */
	Loop_RTC0(Mode mode = Mode::POLL);
	~Loop_RTC0() override;

	void run() override;
	[[nodiscard]] Time now() override;
	[[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;
	using Loop::sleep;

protected:
	Mode mode;

	// base time for now() because the RTC counter is only 24 bit and runs at 16384Hz
	int32_t baseTime = 0;
};

} // namespace coco
