/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/1/23, v1.0 create this file.
*******************************************************************************/

#include "osapi.h"
#include "at_custom.h"
#include "user_interface.h"

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "espconn.h"

#include "uart.h"

#include "gpio.h"

 uint8_t led_i = 0;
 uint8_t led_blink_cnt =0;
 uint8_t gpio0_triger_down = 0;
 LOCAL void  gpio_intr_handler(int * dummy);

void ICACHE_FLASH_ATTR 
 gpio0_intr_init(void)
{
	// Initialize the GPIO subsystem.
   gpio_init();
   PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
   // You can select enable/disable internal pullup/pulldown for each pin
//
//  PIN_PULLUP_DIS(PIN_NAME)
//  PIN_PULLUP_EN(PIN_NAME)
//  PIN_PULLDWN_DIS(PIN_NAME)
//  PIN_PULLDWN_EN(PIN_NAME)

    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
   gpio_output_set(0, 0, 0, BIT0);//set gpio 0 as input

       // Disable interrupts by GPIO
    ETS_GPIO_INTR_DISABLE();
	   
	   // Attach interrupt handle to gpio interrupts.
// You can set a void pointer that will be passed to interrupt handler each interrupt

    ETS_GPIO_INTR_ATTACH(gpio_intr_handler, &gpio0_triger_down);

// All people repeat this mantra but I don't know what it means
//
     gpio_register_set(GPIO_PIN_ADDR(0),
                       GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)  |
                       GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE) |
                       GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

// clear gpio status. Say ESP8266EX SDK Programming Guide in  5.1.6. GPIO interrupt handler

     GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(0));

// enable interrupt for his GPIO
//     GPIO_PIN_INTR_... defined in gpio.h

     gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_ANYEDGE);

     ETS_GPIO_INTR_ENABLE();

}

//-------------------------------------------------------------------------------------------------
// interrupt handler
// this function will be executed on any edge of GPIO0
LOCAL void  gpio_intr_handler(int * dummy)
{
// clear gpio status. Say ESP8266EX SDK Programming Guide in  5.1.6. GPIO interrupt handler

    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

// if the interrupt was by GPIO0
    if (gpio_status & BIT(0))
    {
// disable interrupt for GPIO0
        gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_DISABLE);

// Do something, for example, increment whatyouwant indirectly
       (*dummy) = 1;



//clear interrupt status for GPIO0
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(0));

// Reactivate interrupts for GPIO0
        gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_ANYEDGE);
    }
}


void ICACHE_FLASH_ATTR
led_timer_cb(void *timer_arg)
{
	if(gpio0_triger_down)
	if (led_i == 0)
	{
		gpio_output_set(BIT2, 0, BIT2, 0x00);

			

	}
	else
	{
		// 每一次都要对输出引脚使能
				gpio_output_set(0, BIT2, BIT2, 0x00);
		
		led_blink_cnt ++;
		if(led_blink_cnt == 5)
			gpio0_triger_down = 0;
	}
	led_i= !led_i;
}
 os_timer_t led_timer;

void ICACHE_FLASH_ATTR user_led_blink_test_init(void)
{
	gpio_init();

 	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	gpio_output_set( 0, BIT2, BIT2, 0x00);//输出使能

	 os_timer_disarm(&led_timer);//取消定时器.定时
	os_timer_setfn(&led_timer, (os_timer_func_t *)led_timer_cb, (void *)1);
	//设置定时器.回调函数。使.用定时器.，必须设置回调函数。
	os_timer_arm(&led_timer, 100, 1);//使能毫秒级定时器.

}

void init_done_cb(void)
{
	char buf[64] = {0};

	os_sprintf(buf, "compile time:%s %s", __DATE__, __TIME__);

	os_printf("uart init ok, %s\n", buf);
}
void user_rf_pre_init(void)
{
}
void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	user_led_blink_test_init();
	gpio0_intr_init();
	system_init_done_cb(init_done_cb);
}

