#include "osapi.h"
#include "at_custom.h"
#include "user_interface.h"

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "espconn.h"

#include "cmd_esp8266.h"


/*****************************************************************************
 * 因为这几个函数无法直接放在CmdTbl中调用，所以重新封装一次
 */
void ICACHE_FLASH_ATTR *_malloc(uint32_t size)
{
	return (void *)os_malloc(size);
}
void ICACHE_FLASH_ATTR _free(void *p)
{
	os_free(p);
}
void ICACHE_FLASH_ATTR *_memset(void *s, int c, uint32_t n)
{
	os_memset(s, c, n);
	return (void *)s;
}
void ICACHE_FLASH_ATTR delay_us(uint32_t us)
{
	os_delay_us(us);
}
void ICACHE_FLASH_ATTR get_ip_mac(void)
{
	char hwaddr[6];
	struct ip_info ipconfig;

	wifi_get_ip_info(SOFTAP_IF, &ipconfig);
	wifi_get_macaddr(SOFTAP_IF, hwaddr);
	os_printf("soft-AP:" MACSTR " " IPSTR, MAC2STR(hwaddr), IP2STR(&ipconfig.ip));

	wifi_get_ip_info(STATION_IF, &ipconfig);
	wifi_get_macaddr(STATION_IF, hwaddr);
	os_printf("\nstation:" MACSTR " " IPSTR "\n", MAC2STR(hwaddr), IP2STR(&ipconfig.ip));
}




/******************************************************************************
 *  gpio高低电平测试
 * 测试goio0,2,4,5,9,10,12,13,14,15,16
 * gpio1,3用作串口
 * gpio6,7,8,11用作spi flash的DIO
 */
#define GPIO_TEST_IO_BITS  (BIT0|BIT2|BIT12|BIT13|BIT14|BIT15)

os_timer_t gpio_output_test_tm;

static void ICACHE_FLASH_ATTR
gpio_output_test_cb(void *timer_arg)
{
	static uint8_t i = 0;
	if (i == 0)
	{
		//置1
		gpio_output_set(GPIO_TEST_IO_BITS, 0, GPIO_TEST_IO_BITS, 0);
		gpio16_output_set(1);
	}
	else
	{
		// 每一次都要对输出引脚使能
		gpio_output_set(0, GPIO_TEST_IO_BITS, GPIO_TEST_IO_BITS, 0);//清0
		gpio16_output_set(0);
	}
	i = !i;
}
void ICACHE_FLASH_ATTR gpio_output_test_init(void)
{
	// 选择为gpio功能
	gpio16_output_conf();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
	gpio_output_set(0, 0, GPIO_TEST_IO_BITS, 0x00);//输出使能

	// 开始高低电平测试
	os_timer_disarm(&gpio_output_test_tm);
	os_timer_setfn(&gpio_output_test_tm, (os_timer_func_t *)gpio_output_test_cb, 0);
	os_timer_arm(&gpio_output_test_tm, 2000, 1);
}
void ICACHE_FLASH_ATTR gpio_output_test_exit(void)
{
	os_timer_disarm(&gpio_output_test_tm);
}




/******************************************************************************
 * gpio输入测试
 */
#define BUTTON_DOWN_FLAG	0x80
#define BUTTON_HOLD_FLAG	0x40
#define BUTTON_UP_FLAG		0x20

uint8_t key_value[10];
uint8_t key_flag[10];
os_timer_t gpio_input_test_tm;

char *key_name[]=
{
"gpio0",
"gpio2",
"gpio4",
"gpio5",
"gpio9",
"gpio10",
"gpio12",
"gpio13",
"gpio14",
"gpio15",
};

static void ICACHE_FLASH_ATTR
_scan(uint8_t i, uint8_t newvalue)
{
	if (newvalue)
	{
		//按下
		if (!key_value[i])
		{
			//0->1
			key_flag[i] |= BUTTON_DOWN_FLAG;
		}
		else
		{
			//1->1
			key_flag[i] |= BUTTON_HOLD_FLAG;
		}
	}
	else
	{
		//松开
		if (key_value[i])
		{
			//1->0
			key_flag[i] |= BUTTON_UP_FLAG;
		}
		else
		{
			//0->0,抖动
		}
	}
	key_value[i] = newvalue;
}
LOCAL void ICACHE_FLASH_ATTR
gpio_poll_cb(void *timer_arg)
{
	uint32 v = gpio_input_get();

	//扫描按键
	_scan(0, (v&(1<<0))? 0:1);//gpio0
	_scan(1, (v&(1<<2))? 0:1);//gpio2
	_scan(2, (v&(1<<4))? 0:1);//gpio4
	_scan(3, (v&(1<<5))? 0:1);//gpio5
	_scan(4, (v&(1<<9))? 0:1);//gpio9
	_scan(5, (v&(1<<10))? 0:1);//gpio10
	_scan(6, (v&(1<<12))? 0:1);//gpio12
	_scan(7, (v&(1<<13))? 0:1);//gpio13
	_scan(8, (v&(1<<14))? 0:1);//gpio14
	_scan(9, (v&(1<<15))? 0:1);//gpio15
	
	//判断标志位
	uint8_t i;
	for (i=0; i<10; i++)
	{
		if (key_flag[i] & BUTTON_DOWN_FLAG)
		{
			os_printf("%s: down\n", key_name[i]);
		}
		else if (key_flag[i] & BUTTON_UP_FLAG)
		{
			os_printf("%s: up\n", key_name[i]);
		}

		//清掉标志位
		key_flag[i] = 0;
	}
}
void ICACHE_FLASH_ATTR gpio_input_test_init(void)
{
	// 选择为gpio功能
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
	gpio_output_set(0, 0, 0, (BIT0|BIT2|BIT4|BIT5|BIT9|BIT10|BIT12|BIT13|BIT14|BIT15));//输入使能
	
	//上拉使能
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_SD_DATA2_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_SD_DATA3_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDI_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTCK_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTMS_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDO_U);

	os_timer_disarm(&gpio_input_test_tm);
	os_timer_setfn(&gpio_input_test_tm, (os_timer_func_t *)gpio_poll_cb, 0);
	os_timer_arm(&gpio_input_test_tm, 50, 1);
}
void ICACHE_FLASH_ATTR gpio_input_test_exit(void)
{
	os_timer_disarm(&gpio_input_test_tm);
}




