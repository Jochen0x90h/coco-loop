#include "Loop_TIM.hpp"
//#include <coco/debug.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

// maximum sleep time is 32.767 seconds
constexpr int MAX_SLEEP = 0x7fff;


Loop_TIM::Loop_TIM(const timer::Info1 &timerInfo, int prescaler, Mode mode)
	: timer(timerInfo.timer), timerIrq(timerInfo.irq), mode(mode)
{
	// disabled interrupts trigger an event and wake up the processor from WFE
	// stm32f0: see chapter 5.3.3 in reference manual (https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
	if (mode == Mode::WAIT)
		SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

	// enable clock
	timerInfo.rcc.enableClock();

	// initialize timer to 1kHz
	auto timer = timerInfo.timer;
	timer->PSC = prescaler; // prescaler for 1ms timer resolution
	timer->ARR = 0xffff;
	//timer->CCR1 = MAX_SLEEP;
	timer->EGR = TIM_EGR_UG; // update generation so that prescaler takes effect
	timer->DIER = TIM_DIER_CC1IE; // interrupt enable for CC1
	timer->CR1 = TIM_CR1_CEN; // start counter
}

Loop_TIM::~Loop_TIM() {
}

void Loop_TIM::run() {
	// enable watchdog
	if (this->mode == Mode::WATCHDOG)
		IWDG->KR = 0xCCCC;

	// set watchdog registers
	//IWDG->KR = 0x5555;
	//while (IWDG->SR != 0);

	Time currentTime = now();
	while (!this->exitFlag) {
		auto timer = this->timer;

		// restart watchdog
		IWDG->KR = 0xAAAA;

		// wait for event if sleep time has not yet passed
		// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
		if (this->mode == Mode::WAIT) {
			// get sleep time (point in time when the first task is due)
			Time sleepTime = this->sleepTasks2.getFirstTime(this->sleepTasks1.getFirstTime(currentTime + MAX_SLEEP * 1ms));

			// set new timeout and clear pending interrupt flags at peripheral and NVIC
			int32_t timeout = sleepTime.value & 0xffff; // lower 16 bit are relevant
			timer->CCR1 = timeout;
			timer->SR = ~TIM_SR_CC1IF;
			nvic::clear(this->timerIrq);

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
		Handler *handler;
		while ((handler = this->handlerQueue.pop()) != nullptr) {
			handler->handle();
		}

		// resume coroutines waiting on sleep()
		currentTime = now();
		this->sleepTasks1.doUntil(currentTime);
		this->sleepTasks2.doUntil(currentTime);
	}
	this->exitFlag = false;
}

Loop::Time Loop_TIM::now() {
	//return Time(TIM2->CNT);
	auto &timer = this->timer;

	uint32_t counter = timer->CNT;
	if (timer->SR & TIM_SR_UIF) {
		timer->SR = ~TIM_SR_UIF;

		// reload counter in case overflow happened after reading the counter
		counter = timer->CNT;

		// advance base time by 65536
		this->baseTime += 0x10000;
	}
	return Time(this->baseTime + counter);
}

Awaitable<CoroutineTimedTask> Loop_TIM::sleep(Time time) {
	return {this->sleepTasks2, time};
}

} // namespace coco
