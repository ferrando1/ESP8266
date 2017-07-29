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
#include "sntp.h"


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

void ICACHE_FLASH_ATTR 
user_led_blink_test_init(void)
{
	gpio_init();

 	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	gpio_output_set( 0, BIT2, BIT2, 0x00);//输出使能

	 os_timer_disarm(&led_timer);//取消定时器.定时
	os_timer_setfn(&led_timer, (os_timer_func_t *)led_timer_cb, (void *)1);
	//设置定时器.回调函数。使.用定时器.，必须设置回调函数。
	os_timer_arm(&led_timer, 100, 1);//使能毫秒级定时器.

}

void ICACHE_FLASH_ATTR
esp8266_wifi_configure(void)
{
	//设置ESP8266 wifi工作模式为station+AP;
	wifi_set_opmode(0x03);
	
	//设置AP模式下的SSID
	struct softap_config softap_conf;
	uint8_t AP_ssid[32] ="Athony"; 
	os_memcpy(softap_conf.ssid, AP_ssid, sizeof(AP_ssid));
	softap_conf.ssid_len=0;
	softap_conf.max_connection = 4;
	wifi_softap_set_config(&softap_conf);
//	if(wifi_softap_dhcps_status() ==DHCP_STARTED)
//		os_printf("wifi softAP DHCP start\n");

	//	设置Station模式下连接路由器热点
	uint8_t station_ssid[32] = "Joder";
	uint8_t password[64] = "531531531";
	struct station_config station_conf;
		//need not mac address
	station_conf.bssid_set= 0;
	os_memset(&station_conf, 0, sizeof(struct station_config));
	os_memset(station_conf.password, 0, sizeof(station_conf.password));
	os_memcpy(station_conf.password, password, os_strlen(password));
	os_strncpy(station_conf.ssid, station_ssid, sizeof(station_ssid));
	if(wifi_station_set_config(&station_conf))
		os_printf("set up device to connect router:%s\n", station_conf.ssid);
	//设置连接路由失败或断开重连
	wifi_station_set_reconnect_policy(true);
	
	//打印路由器显示的esp8266用户名
//	os_printf("ESP8266 station host name:%s\n",wifi_station_get_hostname());
	//修改sation模式下的路由器显示用户名
	wifi_station_set_hostname("Bruto");
		
}

/*****************************************************************************************
 *  DNS 解析IP
 */
ip_addr_t iot_server_ip;
LOCAL struct espconn dns_espconn;
os_timer_t ip_analysis_timer;

// 
LOCAL void ICACHE_FLASH_ATTR
dns_ip_analysis_callback(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *dns_espconn = (struct espconn *)arg;
	if (ipaddr == NULL)
	{
//		os_printf("dns_ip_analysis: NULL\n");
		os_timer_arm(&ip_analysis_timer, 1000, 0);//使能毫秒级定时器.
	}
	else
	{
		os_printf("dns_ip_analysis: %d.%d.%d.%d\n",
		          *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
		          *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
	os_timer_disarm(&ip_analysis_timer);//取消定时器定时

	}
}


//DNS通过域名解析出IP地址
void ICACHE_FLASH_ATTR
dns_ip_analysis(const char *hostname)
{
	//实际应用时可以每隔1s执行一次, dns_found成功之后，停止定时器即可
	//dns_found失败的话,切换ap，再次进行dns解析
	espconn_gethostbyname(&dns_espconn, hostname, &iot_server_ip, dns_ip_analysis_callback);


}

uint8_t ip_analysis_check_flag = 1;

// 使用软定时器每隔1s 解析一次IP，这样的好处是如果只解析一次，可能无法解析到IP，因此需要多次解析
 void ICACHE_FLASH_ATTR
 dns_ip_analysis_test_timer_callback(void)
 {
 //if(ip_analysis_check_flag )
 	dns_ip_analysis("ferrando.win");
			//如果解析到IP地址，就停止解析
//	if(&iot_server_ip == NULL)
//		 ip_analysis_check_flag = 0;//如果解析到IP地址，就停止解析
 }

 void ICACHE_FLASH_ATTR
 dns_ip_analysis_test_timer(void)
 	{
 os_timer_disarm(&ip_analysis_timer);//取消定时器定时
os_timer_setfn(&ip_analysis_timer, (os_timer_func_t *)dns_ip_analysis_test_timer_callback, (void *)1);
//设置定时器.回调函数。使.用定时器.，必须设置回调函数。
os_timer_arm(&ip_analysis_timer, 1000, 0);//使能毫秒级定时器.
 	}


//IP解析测试
void ICACHE_FLASH_ATTR
dns_ip_analysis_test(void)
{
	dns_ip_analysis("ferrando.win");
	dns_ip_analysis_test_timer();
}

/*****************************************************************************
 * sntp(简单网络时间协议)测试
 * now sdk support SNTP_MAX_SERVERS = 3
 * 202.120.2.101 (上海交通大学网络中心NTP服务器地址）
 * 210.72.145.44 (国家授时中心服务器)
 */
 //打印出从SNTP授时服务器中获得的实际时间
void ICACHE_FLASH_ATTR
get_real_time(void)
{
	//查询当前距离基准时间（1970.01.01 00 ：00：00 GMT + 8）的时间戳，单位：秒
	uint32 t = sntp_get_current_timestamp();
	//查询实际时间 (GMT + 8)
	os_printf("sntp:%s\n", sntp_get_real_time(t));
}
//sntp软定时器callback声明
void ICACHE_FLASH_ATTR sntp_user_check_stamp_callback(void *arg);
LOCAL os_timer_t sntp_timer;
//SNTP网络同步授时服务器配置	
void ICACHE_FLASH_ATTR
sntp_config(void)
{
	ip_addr_t ip;

	//SNTP 关闭
	//sntp_stop();
	//设置时区信息 sint8 timezone：时区值，参数范围：-11 ~ 13
	sntp_set_timezone(8);
	//Set an IP address given by the four byte-parts
//	IP4_ADDR(&ip, 202, 120, 2, 101);
	//查询 SNTP 服务器.的域名
	//sntp_setservername(0, ”us.pool.ntp.org”); // set server 0 by domain name
	//通过 IP 地址设置 SNTP 服务器，一共最多支持设置 3 个 SNTP 服务器.
	//0 号为主服务器，1 号和 2 号为备用服务器.。
//	sntp_setserver(0, &ip);
	//通过 IP 地址设置 SNTP 服务器1
//	IP4_ADDR(&ip, 210, 72, 145, 44);
//	sntp_setserver(1, &ip);
	sntp_setservername(0,"0.cn.pool.ntp.org");	
	 sntp_setservername(1,"1.cn.pool.ntp.org");  
	 sntp_setservername(2,"2.cn.pool.ntp.org");  

	//SNTP 初始化，并启动SNTP
	sntp_init();
	
os_timer_disarm(&sntp_timer);
os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntp_user_check_stamp_callback, NULL);
os_timer_arm(&sntp_timer, 3000, 0);
	//打印出从SNTP授时服务器中获得的时间
//	get_real_time();
}

void ICACHE_FLASH_ATTR 
sntp_user_check_stamp_callback(void *arg){
uint32 current_stamp;
current_stamp = sntp_get_current_timestamp();
if(current_stamp == 0){
	os_timer_arm(&sntp_timer, 3000, 0);
	} else{
	os_timer_disarm(&sntp_timer);
	os_printf("sntp: %d, %s \n",current_stamp,
	sntp_get_real_time(current_stamp));
	}
}


void init_done_cb(void)
{
	char buf[64] = {0};

	os_sprintf(buf, "compile time:%s %s", __DATE__, __TIME__);

	os_printf("uart init ok, %s\n", buf);
	os_printf("SDK version: %s\n", system_get_sdk_version());
	//查询ESP8266芯片ID
	os_printf("Chip ID: 0x%x\n", system_get_chip_id());
	//查询flash ID
	os_printf("Flash ID: 0x%x\n",spi_flash_get_id());
	//查询ESP8266 Wifi工作模式 0x01为Station模式 0x02为SoftAP模式 0x03为Station+AP模式
	os_printf("Wifi current mode:0x%x\n",wifi_get_opmode());
	
}
void user_rf_pre_init(void)
{
}
void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	user_led_blink_test_init();
	gpio0_intr_init();
	
	esp8266_wifi_configure();
	
	dns_ip_analysis_test();

	sntp_config();
	
	system_init_done_cb(init_done_cb);
}

