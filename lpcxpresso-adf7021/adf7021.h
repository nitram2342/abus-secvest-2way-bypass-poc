/*
 * adf7021.h
 *
 *  Created on: Apr 6, 2012
 *      Author: martin
 */

#ifndef ADF7021_H_
#define ADF7021_H_

#include "globals.h"

#define TXRX_CLOCK_RATE 8092

uint16_t adf7021_read_silicon_revision();
void adf7021_init_chip();
void adf7021_init_interface();
void adf7021_mode_rx();
void adf7021_mode_tx();

int adf7021_in_rx_mode();

int adf7021_set_test_mode(int mode);

#endif /* ADF7021_H_ */
