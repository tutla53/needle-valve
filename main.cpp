/* Soft Robot Servo-Valve Actuator
 * by: Tutla Ayatullah
 
 * Hardware:- STM32F103C8T6 (bluepill)
			- Parallax Standard Servo
			- Needle Valve
			- MPX5100DP or MPX5500DP Pressure Sensor
			
 * Version:	1. Sweep Servo Motor (PB0 == TIM3.CH3)
			2. Add Button to Start and Stop Servo Sweep
 */
 
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#define mainECHO_TASK_PRIORITY				( tskIDLE_PRIORITY + 1 )

#define min_pulse 	550
#define max_pulse	2350

/* All pins use GPIOA port*/
#define startPin	GPIO2	// I: Start Button
#define stopPin		GPIO3	// I: Stop Button

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,signed portCHAR *pcTaskName);

void vApplicationStackOverflowHook(xTaskHandle *pxTask,signed portCHAR *pcTaskName){
	(void)pxTask;
	(void)pcTaskName;
	for(;;);
}

static void gpio_setup(void) {
	
	rcc_clock_setup_in_hse_8mhz_out_72mhz();	// Use this for "blue pill"
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set_mode(GPIOC,GPIO_MODE_OUTPUT_2_MHZ,GPIO_CNF_OUTPUT_PUSHPULL,GPIO13);
	gpio_set_mode(GPIOA,GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, startPin|stopPin);
	
}

static void pwm_setup(void){

	rcc_periph_clock_enable(RCC_TIM3);		// Need TIM3 clock
	rcc_periph_clock_enable(RCC_AFIO);		// Need AFIO clock

	// PB0 == TIM3.CH3	
	rcc_periph_clock_enable(RCC_GPIOB);		// Need GPIOB clock
	gpio_primary_remap(
		AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF,	// Optional
		AFIO_MAPR_TIM2_REMAP_NO_REMAP);		// This is default: TIM3.CH3=GPIOB0
	gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_50_MHZ,	// High speed
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,GPIO0);	// GPIOB0=TIM3.CH3

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
	timer_set_period(TIM3,20000-1);

	timer_disable_oc_output(TIM3,TIM_OC3);
	timer_set_oc_mode(TIM3,TIM_OC3,TIM_OCM_PWM1);
	timer_enable_oc_output(TIM3,TIM_OC3);

	timer_set_oc_value(TIM3,TIM_OC3,min_pulse);
	timer_enable_counter(TIM3);
}

static void task1(void *args __attribute__((unused))) {
	
	bool startStat = 0, stopStat = 0, sysStart = 0;
	
	for (;;) {
		startStat = gpio_get(GPIOA,startPin);
		stopStat =gpio_get(GPIOA, stopPin);
		
		if(startStat && !stopStat  && !sysStart) sysStart = 1;
		if(stopStat  && !startStat && sysStart) sysStart = 0;
		
		if(sysStart){
			gpio_clear(GPIOC,GPIO13);
			
			for (int i = min_pulse; i <= max_pulse; i=i+5){
				timer_set_oc_value(TIM3,TIM_OC3,i);
				vTaskDelay(pdMS_TO_TICKS(2));
			}
			vTaskDelay(pdMS_TO_TICKS(500));
		
			gpio_toggle(GPIOC,GPIO13);
			for (int i = max_pulse; i >= min_pulse; i=i-5){
				timer_set_oc_value(TIM3,TIM_OC3,i);
				vTaskDelay(pdMS_TO_TICKS(2));
			}		
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		
		else{
			gpio_set(GPIOC, GPIO13);
		}
	}
}

int main(void) {

	gpio_setup();
	pwm_setup();
	xTaskCreate(task1,"task1",100,NULL,1,NULL);
	vTaskStartScheduler();
	for (;;)
		;
	return 0;
}