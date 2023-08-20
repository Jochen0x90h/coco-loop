#include "Loop_TIM2.hpp"
//#include <coco/debug.hpp>
#include <coco/platform/platform.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

Loop_TIM2::Loop_TIM2(int prescaler, Mode mode) : mode(mode) {
	// disabled interrupts trigger an event and wake up the processor from WFE
	// stm32f0: see chapter 5.3.3 in reference manual (https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
	if (mode == Mode::WAIT)
		SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

	// initialize TIM2 to 1kHz
#ifdef STM32F0
	RCC->APB1ENR = RCC->APB1ENR | RCC_APB1ENR_TIM2EN;
#endif
#ifdef STM32G4
	RCC->APB1ENR1 = RCC->APB1ENR1 | RCC_APB1ENR1_TIM2EN;
#endif
	TIM2->PSC = prescaler; // prescaler for 1ms timer resolution
	TIM2->EGR = TIM_EGR_UG; // update generation so that prescaler takes effect
	TIM2->DIER = TIM_DIER_CC1IE; // interrupt enable for CC1
	TIM2->CR1 = TIM_CR1_CEN; // enable, count up
}

Loop_TIM2::~Loop_TIM2() {
}

void Loop_TIM2::run(const int &condition) {
	// enable watchdog
	if (this->mode == Mode::WATCHDOG)
		IWDG->KR = 0xCCCC;

	// set watchdog registers
	//IWDG->KR = 0x5555;
	//while (IWDG->SR != 0);

	int c = condition;
	Time currentTime = now();
	while (c == condition) {
		// restart watchdog
		IWDG->KR = 0xAAAA;

		// get sleep time
		Time sleepTime = this->sleepTasks2.getFirstTime(this->sleepTasks1.getFirstTime(currentTime + Duration::max() / 2));

		// set new timeout and clear pending interrupt flags at peripheral and NVIC
		TIM2->CCR1 = sleepTime.value;
		TIM2->SR = ~TIM_SR_CC1IF;
		nvic::clear(TIM2_IRQn);
		bool notPassed = int32_t(sleepTime.value - TIM2->CNT) > 0;

		// wait for event if sleep time has not yet passed
		// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
		if (this->mode == Mode::WAIT && notPassed) {
			//debug::toggleGreen();

			// data synchronization barrier
			__DSB();

			// wait for event (interrupts trigger an event due to SEVONPEND)
			__WFE();
		}

		// call all handlers
		auto it = this->handlers.begin();
		while (it != this->handlers.end()) {
			// increment iterator beforehand because a handler can remove() itself
			auto current = it;
			++it;
			current->handle();
		}

		// resume coroutines waiting on sleep()
		currentTime = now();
		this->sleepTasks1.doUntil(currentTime);
		this->sleepTasks2.doUntil(currentTime);
	}
}

Time Loop_TIM2::now() {
	return {TIM2->CNT};
}

Awaitable<CoroutineTimedTask> Loop_TIM2::sleep(Time time) {
	return {this->sleepTasks2, time};
}

/*
void sleepBlocking(int us) {
	auto cycles = us * 8;
	for (int i = 0; i < cycles; ++i) {
		__NOP();
	}
}*/

} // namespace coco
