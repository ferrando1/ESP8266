#ifndef _FCMD_CFG_H_
#define _FCMD_CFG_H_
/*******************************************************************************
* 用户函数命令头文件包含，函数声明
*/

#include "cmd_mem.h"
#include "osapi.h"

#include "user_interface.h"
#include "mem.h"
#include "cmd_esp8266.h"
#include "flash_api.h"
#include "gpio.h"
#include "gpio16.h"


/*******************************************************************************
 * 自定义函数命令表
 */
CmdTbl_t CmdTbl[] =
{
	//系统命令, SYSTEM_CMD_NUM和系统命令个数保持一致
	"ls",  0,
	"addr", 0,
	"help", 0,


	//用户命令
	"void md(int addr, int elem_cnt, int elem_size)", (void(*)(void))md,
	"int cmp(void *addr1, void *addr2, int elem_cnt, int elem_size)", (void(*)(void))cmp,

	//system api
	"uint32 system_get_free_heap_size(void)", (void(*)(void))system_get_free_heap_size,
	"uint32 system_get_chip_id(void)", (void(*)(void))system_get_chip_id,
	"void system_restart(void)", (void(*)(void))system_restart,
	"void system_restore(void)",  (void(*)(void))system_restore,

	"void get_ip_mac(void)", (void(*)(void))get_ip_mac,

	"void *_malloc(uint32_t size)", (void(*)(void))_malloc,
	"void _free(void *p)", (void(*)(void))_free,
	"void *_memset(void *s, int c, uint32_t n)",  (void(*)(void))_memset,
	"void delay_us(uint32_t us)",	(void(*)(void))delay_us,

	// flash
	"uint32 spi_flash_get_id(void)",   (void(*)(void))spi_flash_get_id,
	"int sfmd(u32 addr, int elem_cnt, int elem_size)",	(void(*)(void))sfmd,
	"int sfmw(u32 writeaddr, u8 *pbuf, u32 num)",		(void(*)(void))sfmw,

	"void gpio16_output_conf(void)", (void(*)(void))gpio16_output_conf,
	"void gpio16_output_set(uint8 value)", (void(*)(void))gpio16_output_set,
	"void gpio16_input_conf(void)", (void(*)(void))gpio16_input_conf,
	"uint8 gpio16_input_get(void)", (void(*)(void))gpio16_input_get,
	"uint32 gpio_input_get(void)",  (void(*)(void))gpio_input_get,
	"void gpio_output_set(uint32 set_mask,uint32 clear_mask,uint32 enable_mask,uint32 disable_mask)",
	(void(*)(void))gpio_output_set,
	
	"void gpio_output_test_init(void)",	(void(*)(void))gpio_output_test_init,
	"void gpio_output_test_exit(void)",	(void(*)(void))gpio_output_test_exit,
	"void gpio_input_test_init(void)",	(void(*)(void))gpio_input_test_init,
	"void gpio_input_test_exit(void)",(void(*)(void))gpio_input_test_exit,

};
uint8_t CmdTblSize = sizeof(CmdTbl) / sizeof(CmdTbl[0]);

#endif


