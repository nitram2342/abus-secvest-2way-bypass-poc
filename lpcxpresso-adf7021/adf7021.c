/*
 * adf7021.c
 *
 *  Created on: Apr 6, 2012
 *      Author: martin
 */

#include "globals.h"
#include "lpc17xx_gpio.h"

#include <stdio.h>
#include "adf7021.h"

int in_rx_mode = TRUE;

static const uint32_t adf7021_defaults_settings_rx = 0x19611300;
static const uint32_t adf7021_defaults_settings_tx = 0x11611D60;

static const uint32_t adf7021_defaults_settings[16] = {
		  0, 		// depends on rx / tx
		  0x535011, // 1 -- VCO/OSCILLATOR REGISTER
		  0x8DD082, // 2 -- TRANSMIT MODULATION REGISTER
		  0x2B1498A3, // 3 -- TRANSMIT/RECEIVE CLOCK REGISTER
		  0x8047B014, //0x80500004, // 4 -- DEMODULATOR SETUP REGISTER

		  0x03155, // 5 -- IF FILTER SETUP REGISTER

		  0x50972C6, // 6 -- IF FINE CAL SETUP REGISTER
		  0x007, // 7
		  0x0008, // 8
		  0x231E9, // 9 -- AGC REGISTER
		  0x3296354A, // 10 -- AFC REGISTER

		  0xf0f0f33B, // 11 - swd
		  0x01B8C, // 12

		  0x0000D, // 13
		  0x0000000E, // 14
		  0x0000000F, // 15
};



#define adf7021_sclk_lo() p0_low(22)
#define adf7021_sclk_hi() p0_high(22)

#define adf7021_ce_lo() p0_low(3)
#define adf7021_ce_hi() p0_high(3)


#define adf7021_sle_lo()  p0_low(21)
#define adf7021_sle_hi()  p0_high(21)

#define adf7021_sdata_lo() p2_low(13)
#define adf7021_sdata_hi() p2_high(13)

#define adf7021_get_sread() p0_read(28)


void adf7021_sleep(int t) {
	int i;
	for(i = 0; i < t; i++) {
		;
	}
}

void adf7021_serial_shift(uint32_t val, int msb_pos) { // val|reg in MSB

	int i;
	// last 4 bits contains the register number
	int reg_idx = val & 0xf;

	printf("  write value 0x%x into reg %d\n", val, reg_idx);
	// initially, set clock pin to low
	adf7021_sclk_lo();
	adf7021_sle_lo();

	for(i = msb_pos; i >= 0; i--) {

		if(val & (1 << i)) {
			adf7021_sdata_hi();
		}
		else {
			adf7021_sdata_lo();
		}

		// rising edge of slck
		adf7021_sclk_hi();
		adf7021_sleep(1000);
		adf7021_sclk_lo();
		adf7021_sleep(1000);
	}

	adf7021_sle_hi();
}

void adf7021_write_config_register(uint32_t val) { // val|reg in MSB
	adf7021_serial_shift(val, 31);
    adf7021_sle_lo();
}


uint16_t adf7021_read_config_register(int readback, int readback_mode, int adc_mode) {
	uint32_t rb = 0;
	uint16_t ret = 0;
	int i;

	rb |= readback;

	rb <<= 2;
	rb |= readback_mode;

	rb <<= 2;
	rb |= adc_mode;

	rb <<= 4;
	rb |= 7; // append register index

	adf7021_serial_shift(rb, 8);
	adf7021_sleep(1000);

	adf7021_sclk_hi();
	adf7021_sleep(1000);
	adf7021_sclk_lo();

	for(i = 0; i < 16; i++) {
		ret <<= 1;
		adf7021_sclk_hi();
		adf7021_sleep(1000);
		int v = adf7021_get_sread();
		ret |= v;
		adf7021_sclk_lo();
	}

    adf7021_sle_lo();
    adf7021_sleep(1000);

    // extra clock cycle
    adf7021_sclk_hi();
    adf7021_sleep(1000);
	adf7021_sclk_lo();

	return ret;
}

uint16_t adf7021_read_silicon_revision() {
	return adf7021_read_config_register(1, 3, 0);
}

void adf7021_mode_rx() {
	uint32_t val = adf7021_defaults_settings_rx;
	adf7021_write_config_register(val | (1 << 27));

	in_rx_mode = TRUE;
}

void adf7021_mode_tx() {
	uint32_t val = adf7021_defaults_settings_tx;
	adf7021_write_config_register(val & ~(1 << 27));
	in_rx_mode = FALSE;
}

int adf7021_in_rx_mode() {
	return in_rx_mode;
}

int adf7021_set_test_mode(int mode) {
	uint32_t val = adf7021_defaults_settings[15];
	if(mode >= 0 && mode <= 6) {
		val &= ~(7 << 8);
		adf7021_write_config_register(val | (mode << 8));
		return 1;
	}
	else return 0;
}

void adf7021_init_chip() {

	adf7021_write_config_register(adf7021_defaults_settings[15] | (0x7 << 17)); // enable SPI mode in reg 15

	// recommended initialisation order
	adf7021_write_config_register(adf7021_defaults_settings[1]);
	adf7021_write_config_register(adf7021_defaults_settings[3]);
	adf7021_mode_rx(); // writre register 0

	adf7021_write_config_register(adf7021_defaults_settings[2]);

	adf7021_write_config_register(adf7021_defaults_settings[5]);

	adf7021_write_config_register(adf7021_defaults_settings[11]);
	adf7021_write_config_register(adf7021_defaults_settings[12]);

	adf7021_write_config_register(adf7021_defaults_settings[4]);


	adf7021_write_config_register(adf7021_defaults_settings[6]);
	adf7021_write_config_register(adf7021_defaults_settings[9]);
	adf7021_write_config_register(adf7021_defaults_settings[10]);

}

void adf7021_init_interface() {
	// CE: P0.3
	p0_output(3);
	p0_low(3);
	printf("+++ power-cycle adf7021\n");
	p0_high(3);

	// SLE: P0.21
	p0_output(21);

	// SREAD: 0.28
	p0_input(28);


	// SCLK: 0.22
	p0_output(22);

	// SDATA: 2.13
	p2_output(13);

}
