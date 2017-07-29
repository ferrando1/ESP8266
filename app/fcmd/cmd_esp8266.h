#ifndef _CMD_ESP8266_H_
#define _CMD_ESP8266_H_


void *_malloc(uint32_t size);
void _free(void *p);
void *_memset(void *s, int c, uint32_t n);
void delay_us(uint32_t us);
void get_ip_mac(void);


void gpio_output_test_init(void);
void gpio_output_test_exit(void);
void gpio_input_test_init(void);
void gpio_input_test_exit(void);


#endif


