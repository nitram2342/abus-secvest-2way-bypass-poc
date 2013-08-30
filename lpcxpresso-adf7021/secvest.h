/*
 * secvest.h
 *
 *  Created on: Apr 10, 2012
 *      Author: martin
 */

#ifndef SECVEST_H_
#define SECVEST_H_


typedef struct {
	uint8_t data[100];
	uint32_t length;
	uint32_t status;
} radio_message_type;

radio_message_type * get_next_radio_msg(void);

void setup();

void set_rx_max_length(uint32_t len);

unsigned int secvest_tx_raw(uint8_t * buf, unsigned int len);
unsigned int secvest_tx_sv2(uint8_t * buf, unsigned int len);

int hex_digit_to_int(uint8_t c);
unsigned int secvest_load_hexstr(uint8_t * buf, unsigned int max_length, uint8_t * str, unsigned int length);

#endif /* SECVEST_H_ */
