/*
 * helper.c
 *
 *  Created on: Apr 13, 2012
 *      Author: martin
 */

#include "lpc_types.h"
#include <stdio.h>

void print_bits(uint8_t * buf, int len) {
	int i, j;
	for(j = 0; j < len; j++)
		for(i = 7; i >= 0; i--) {
			printf("%d", buf[j] & (1 << i) ? 1 : 0);
		}
	printf("\n");
}

void print_msg(uint8_t * buf, uint32_t len) {
	uint32_t i2;
	for(i2 = 0; i2 < len; i2++) {
		printf("%02x ", buf[i2]);
	}
	printf("\n");
}

void print_buf(uint8_t * buf, uint32_t len) {
	uint32_t i, i2;
	for(i = 0; i+16 < len; i+=16) {
		printf(" %04x  ", i);
		for(i2 = 0; i2 < 16; i2++) {
			printf("%02x ", buf[i+i2]);
		}
		printf("\n");
	}
	if(i < len) {
		printf(" %04x  ", i);
		for(i2 = 0; i+i2 < len; i2++) {
			printf("%02x ", buf[i+i2]);
		}
		printf("\n");
	}
}

int buffer_contains(uint8_t * buf, uint32_t len, uint8_t val) {
	uint32_t i;
	for(i = 0; i < len; i++) {
		if(buf[i] == val &&
		   buf[i+1] == val) return 1;
	}
	return 0;
}
