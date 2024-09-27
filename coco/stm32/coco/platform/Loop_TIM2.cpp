#include "Loop_TIM2.hpp"
//#include <coco/debug.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

// maximum sleep time is 24.8 days
constexpr int MAX_SLEEP = 0x7fffffff;


Loop_TIM2::Loop_TIM2(int prescaler, Mode mode) : mode(mode) {
	// disabled interrupts trigger an event and wake up the processor from WFE
	// stm32f0: see chapter 5.3.3 in reference manual (https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
	if (mode == Mode::WAIT)
		SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

	// enable clock
#ifdef RCC_APB1ENR_TIM2EN
	RCC->APB1ENR = RCC->APB1ENR | RCC_APB1ENR_TIM2EN;
#endif
#ifdef RCC_APB1ENR1_TIM2EN
	RCC->APB1ENR1 = RCC->APB1ENR1 | RCC_APB1ENR1_TIM2EN;
#endif

	// two cycles wait time until peripherals can be accessed, see STM32G4 reference manual section 7.2.17
	__NOP();
	__NOP();

	// initialize timer to 1kHz
	TIM2->PSC = prescaler; // prescaler for 1ms timer resolution
	TIM2->EGR = TIM_EGR_UG; // update generation so that prescaler takes effect
	TIM2->DIER = TIM_DIER_CC1IE; // interrupt enable for CC1
	TIM2->CR1 = TIM_CR1_CEN; // start counter
}

Loop_TIM2::~Loop_TIM2() {
}

void Loop_TIM2::run() {
	// enable watchdog
	if (this->mode == Mode::WATCHDOG)
		IWDG->KR = 0xCCCC;

	// set watchdog registers
	//IWDG->KR = 0x5555;
	//while (IWDG->SR != 0);

	Time currentTime = now();
	while (!this->exitFlag) {
		// restart watchdog
		IWDG->KR = 0xAAAA;

		// wait for event if sleep time has not yet passed
		// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
		if (this->mode == Mode::WAIT) {
			// get sleep time
			Time sleepTime = this->sleepTasks2.getFirstTime(this->sleepTasks1.getFirstTime(currentTime + MAX_SLEEP * 1ms));

			// set new timeout and clear pending interrupt flags at peripheral and NVIC
			TIM2->CCR1 = sleepTime.value;
			TIM2->SR = ~TIM_SR_CC1IF;
			nvic::clear(TIM2_IRQn);

			// wait if timeout has not passed yet
			bool notPassed = sleepTime.value - int(TIM2->CNT) > 0;
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

Loop::Time Loop_TIM2::now() {
	return Time(TIM2->CNT);
}

Awaitable<CoroutineTimedTask> Loop_TIM2::sleep(Time time) {
	return {this->sleepTasks2, time};
}

} // namespace coco
