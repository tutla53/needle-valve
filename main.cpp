/* Soft Robot Servo-Valve Actuator
 * by: Tutla Ayatullah
 
 * Hardware:- STM32F103C8T6 (bluepill)
			- Parallax Standard Servo
			- Needle Valve
			- MPX5100DP or MPX5500DP Pressure Sensor
			
 * Version:	1.Sweep Servo Motor (PB0 == TIM3.CH3) 
 */
 
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#define mainECHO_TASK_PRIORITY				( tskIDLE_PRIORITY + 1 )

#define min_pulse 	400
#define max_pulse	2400

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,signed portCHAR *pcTaskName);

void vApplicationStackOverflowHook(xTaskHandle *pxTask,signed portCHAR *pcTaskName){
	(void)pxTask;
	(void)pcTaskName;
	for(;;);
}

static void pwm_setup(void){

	rcc_periph_clock_enable(RCC_TIM3);		// Need TIM3 clock
	rcc_periph_clock_enable(RCC_AFIO);		// Need AFIO clock

	// PB0 == TIM3.CH3	
	rcc_periph_clock_enable(RCC_GPIOB);		// Need GPIOB clock
	gpio_primary_remap(
		AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF,	// Optional
		AFIO_MAPR_TIM3_REMAP_NO_REMAP);		// This is default: TIM3.CH3=GPIOB0
	gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_50_MHZ,	// High speed
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,GPIO0);		// GPIOB0=TIM2.CH2

	// TIM3:
	timer_disable_counter(TIM3);
	rcc_periph_reset_pulse(RST_TIM3);

	timer_set_mode(TIM3,
		TIM_CR1_CKD_CK_INT,
		TIM_CR1_CMS_EDGE,
		TIM_CR1_DIR_UP);
	timer_set_prescaler(TIM3,72-1); //see https://github.com/ve3wwg/stm32f103c8t6/pull/12/
	// Only needed for advanced timers:
	// timer_set_repetition_counter(TIM3,0);
	timer_enable_preload(TIM3);
	timer_continuous_mode(TIM3);
	timer_set_period(TIM3,20000-1); /*20 ms pwm period*/

	timer_disable_oc_output(TIM3,TIM_OC3);
	timer_set_oc_mode(TIM3,TIM_OC3,TIM_OCM_PWM1);
	timer_enable_oc_output(TIM3,TIM_OC3);

	timer_set_oc_value(TIM3,TIM_OC3, min_value);
	timer_enable_counter(TIM3);
}

static void gpio_setup(void) {
	
	rcc_clock_setup_in_hse_8mhz_out_72mhz();	// Use this for "blue pill"
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC,GPIO_MODE_OUTPUT_2_MHZ,GPIO_CNF_OUTPUT_PUSHPULL,GPIO13);
}

static void task1() {
	static const int delta = (max_pulse-min_pulse)/3;
	static const int ms[4] = { min_pulse, min_pulse+delta, max_pulse-delta, max_pulse};
	int msx = 0;
	
	for (;;) {
		gpio_toggle(GPIOC,GPIO13);
		msx = (msx+1) % 4;
		timer_set_oc_value(TIM2,TIM_OC2,ms[msx]);
	}
}

int main(void) {

	gpio_setup();
	pwm_setup();
	xTaskCreate(task1,"LED",100,NULL,configMAX_PRIORITIES-1,NULL);
	vTaskStartScheduler();
	for (;;)
		;
	return 0;
}