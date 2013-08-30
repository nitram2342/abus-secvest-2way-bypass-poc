
#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "lpc17xx_ssp.h"
#include "lpc17xx_gpio.h"
#include "adf7021.h"
#include "globals.h"
#include "secvest.h"

#include <string.h>
#include <stdio.h>

#define SSP_CHANNEL LPC_SSP1
#define PORT_CS	LPC_GPIO2
#define PIN_MASK_CS (1<<2)


#define ssel_ctl_lo() p0_low(0)
#define ssel_ctl_hi() p0_high(0)
#define trigger_lo() p0_low(2)
#define trigger_hi() p0_high(2)


#define EINT1_CLR  (1 << 1)
#define EINT1_EDGE (1 << 1)
#define EINT2_CLR  (1 << 2)
#define EINT2_EDGE (1 << 2)

// local forward declarations

int32_t my_SSP_Read(LPC_SSP_TypeDef *SSPx, radio_message_type *dataCfg, unsigned int max_length);

// variables
SSP_CFG_Type sspChannelConfig;

#define MAX_RADIO_MSGS 10
radio_message_type radio_messages[MAX_RADIO_MSGS];
volatile uint32_t current_radio_msg = 0;

volatile uint32_t rx_max_length = 26;
volatile uint32_t enable_ssel_clocking = 0;
volatile uint32_t xxxxx = 0;
volatile uint32_t clock_counter = 0;

volatile uint32_t in_capture = 0;

void secvest_capture() {
	in_capture = 1;
	int ret;
	ret = my_SSP_Read(SSP_CHANNEL, &radio_messages[current_radio_msg], rx_max_length);
	if(ret == -1) {
		// error
	}
	else {
		// sspDataConfig.rx_data, sspDataConfig.rx_cnt
		current_radio_msg = (++current_radio_msg) % MAX_RADIO_MSGS;
	}
	in_capture = 0;
}

// EINT1_IRQHandler() is triggerd on ADF7021 sync word detection
void EINT1_IRQHandler (void) {

	LPC_SC->EXTINT |= EINT1_CLR;        // clear interrupt
	if(xxxxx == 0 || adf7021_in_rx_mode() == FALSE) return;

//	if(in_capture) abort_last
	enable_ssel_clocking = 1;

	//NVIC_DisableIRQ(EINT1_IRQn);

	ssel_ctl_hi();
	trigger_hi(); // for debugging
	clock_counter = 0;
	ssel_ctl_lo();

	secvest_capture();

	trigger_lo(); // for debugging

//	ssel_ctl_hi();
	enable_ssel_clocking = 0;


	//NVIC_EnableIRQ(EINT1_IRQn);
}


void EINT2_IRQHandler (void) {
	LPC_SC->EXTINT |= EINT2_CLR;        // clear interrupt

//	NVIC_DisableIRQ(EINT2_IRQn);
	if(enable_ssel_clocking == 1) {
		//trigger_hi();

		if(clock_counter++ == 7) {
			ssel_ctl_hi();
			clock_counter = 0;
			ssel_ctl_lo();
		}
		//trigger_lo();
	}
	else {
		clock_counter = 0;
//		ssel_ctl_hi();
		ssel_ctl_lo();
	}
//	NVIC_EnableIRQ(EINT2_IRQn);
}

void set_rx_max_length(uint32_t len) {
	rx_max_length = len;
}

void setup_ports() {

	// ssel_ctl
	p0_output(0);
	ssel_ctl_hi();

	// trigger
	p0_output(2);
	trigger_lo();
	trigger_hi();
	trigger_lo();


	// SPI
	LPC_PINCON->PINSEL0 |= 0x2<<12;	//SSEL1
	LPC_PINCON->PINSEL0 |= 0x2<<14; //SCK1
	LPC_PINCON->PINSEL0 |= 0x2<<16;	//MISO1
	LPC_PINCON->PINSEL0 |= 0x2<<18;	//MOSI1
}

void setup_irq_ports(void) {

	// EINT2 is triggered by ssel_ctl
	NVIC_DisableIRQ(EINT2_IRQn); // hangs
    LPC_PINCON->PINSEL4 &= ~(0x11 << 24);
    LPC_PINCON->PINSEL4 |= (0x01 << 24);

	LPC_GPIOINT->IO2IntEnR |= (1<<12);
	LPC_SC->EXTPOLAR &= ~(1 << 2); // falling edge
	LPC_SC->EXTMODE |= EINT2_EDGE; // edge triggered

	LPC_SC->EXTINT |= EINT2_CLR;        // clear interrupt
	NVIC_EnableIRQ(EINT2_IRQn); // hangs



	// Sync Word Detection (SWD) triggers EINT1

	NVIC_DisableIRQ(EINT1_IRQn);

    // Set PINSEL4 [21:20] = 01 for P2.11 as EINT1
    LPC_PINCON->PINSEL4 &= ~(0x11 << 22);
    LPC_PINCON->PINSEL4 |= (0x01 << 22);

	// enable irq handler for swd
	//LPC_GPIOINT->IO2IntEnF &= ~(1<<11);
	LPC_GPIOINT->IO2IntEnR |= (1<<11);
	LPC_SC->EXTMODE |= EINT1_EDGE; // edge triggered
	LPC_SC->EXTPOLAR |= (1 << 1); // rising edge
	LPC_SC->EXTINT |= EINT1_CLR;        // clear interrupt


	NVIC_EnableIRQ(EINT1_IRQn);

	NVIC_SetPriority(EINT1_IRQn, 1); // swd
	NVIC_SetPriority(EINT2_IRQn, 0); // clocked ssel

	printf("+++ irq port configured\n");
	xxxxx = 1;
}


int32_t my_SSP_Read(LPC_SSP_TypeDef *SSPx, radio_message_type *dataCfg, unsigned int max_length) {
	uint8_t *rdata8;
    uint32_t tmp;

    dataCfg->length = 0;
    dataCfg->status = 0;

	/* Clear all remaining data in RX FIFO */
	while (SSPx->SR & SSP_SR_RNE){
		tmp = (uint32_t) SSP_ReceiveData(SSPx);
	}

	// Clear status
	SSPx->ICR = SSP_ICR_BITMASK;

	rdata8 = dataCfg->data;

	unsigned int len = MIN(sizeof(dataCfg->data), max_length);

	while(dataCfg->length < len) {

		// Check for any data available in RX FIFO
		while ((SSPx->SR & SSP_SR_RNE) && (dataCfg->length < len)) {
			// Read data from SSP data
			tmp = SSP_ReceiveData(SSPx);

			// Store data to destination
			*(rdata8) = (uint8_t) tmp;
			rdata8++;

			// Increase counter
			dataCfg->length++;
		}
	}

	// save status
	dataCfg->status = SSP_STAT_DONE;

	return dataCfg->length;
}

unsigned int my_SSP_Write(LPC_SSP_TypeDef *SSPx, radio_message_type *dataCfg) {

    uint8_t *wdata8;
    uint32_t stat;
    uint32_t tmp;

    dataCfg->status = 0;

	/* Clear all remaining data in RX FIFO */ // XXX is this necessary?
	while(SSPx->SR & SSP_SR_RNE) {
		tmp = (uint32_t) SSP_ReceiveData(SSPx);
	}

	// Clear status
	SSPx->ICR = SSP_ICR_BITMASK;

	wdata8 = dataCfg->data;

	uint8_t * max_addr = dataCfg->data + dataCfg->length;

	while(wdata8 < max_addr) {

		if ((SSPx->SR & SSP_SR_TNF) && (wdata8 < max_addr)) {
			SSP_SendData(SSPx, *wdata8);
			wdata8++;
		}

		// Check overrun error
		if ((stat = SSPx->RIS) & SSP_RIS_ROR) {
			// save status and return
			dataCfg->status = stat | SSP_STAT_ERROR;
			return (-1);
		}

		// save status
		dataCfg->status = SSP_STAT_DONE;
	}


	return dataCfg->length;

}

unsigned int secvest_tx_raw(uint8_t * buf, unsigned int len) {
	radio_message_type m;
	if(len > sizeof(m.data)) return 0;

	memcpy(m.data, buf, len);
	m.length = len;

	trigger_hi();
			ssel_ctl_hi();
			enable_ssel_clocking = 1; // should better be renamed to enable_ssel_clocking
			trigger_hi();
			ssel_ctl_lo();
	unsigned int ret = my_SSP_Write(SSP_CHANNEL, &m);

	enable_ssel_clocking = 0;
	trigger_lo();
//	ssel_ctl_hi();
	return ret;
}

void setup_spi(void) {
	memset((void *)radio_messages, 0, sizeof(radio_messages));

	SSP_ConfigStructInit(&sspChannelConfig);
	sspChannelConfig.Mode = SSP_SLAVE_MODE;
	sspChannelConfig.ClockRate = TXRX_CLOCK_RATE;

	sspChannelConfig.CPHA = SSP_CPHA_FIRST; sspChannelConfig.CPOL = SSP_CPOL_HI;

	SSP_Init(SSP_CHANNEL, &sspChannelConfig);
	SSP_Cmd(SSP_CHANNEL, ENABLE);

}


void setup() {

	current_radio_msg = 0;
	enable_ssel_clocking = 0;

	printf("+++ setup ports\n");
	setup_ports();
	adf7021_init_interface();

	printf("+++ read adf7021 revision (should be Product Code=0x210, Revision Code=0x4)\n");
	uint16_t revision = adf7021_read_silicon_revision();
	printf("+++ adf7021 revision: 0x%X\n", revision);

	adf7021_init_chip();


	printf("+++ setup spi mode\n");
	setup_spi();

	printf("+++ setup irq\n");
	setup_irq_ports();

}


radio_message_type * get_next_radio_msg(void) {
	int i;
	for(i = 1; i <= MAX_RADIO_MSGS; i++) {
		int idx = (current_radio_msg - i) % MAX_RADIO_MSGS;
		radio_message_type * m = &radio_messages[idx];
		if(m->length > 0) {
			return m;
		}
	}
	return NULL;
}

int hex_digit_to_int(uint8_t c) {
	if(c >= 'a' && c <= 'f') { return 10 + c - 'a'; }
	else if(c >= 'A' && c <= 'F') { return 10 + c - 'A'; }
	else if(c >= '0' && c <= '9') { return c - '0'; }
	else return -1;
}

unsigned int secvest_load_hexstr(uint8_t * buf, unsigned int max_length, uint8_t * str, unsigned int length) {

	unsigned int len = 0;
	uint8_t * p = str;
	int which_nibble = 0;
	uint8_t current_byte = 0;
	while(p < str + length) {
		int v = hex_digit_to_int(*p);
		if(v < 0) return 0;
		else {
			if(which_nibble == 0) {
				current_byte = v << 4;
				which_nibble = 1;
			}
			else {
				current_byte |= v;
				which_nibble = 0;
				if(len < max_length) buf[len] = current_byte;
				else return 0;
				len++;
			}
		}
		p++;
	}
	return len;
}

const uint8_t preamble[] = {0x55, 0x55, 0x55, 0x55, 0x55};
const uint8_t sync_word[] = {0x30, 0xf0, 0xf0, 0xf3};


unsigned int secvest_add_bytes(uint8_t * buf, unsigned int buf_len, unsigned int offset,
			       const uint8_t * additional_data, unsigned int additional_data_len) {

  uint8_t * p = buf + offset;
  const uint8_t * max = p + additional_data_len;
  const uint8_t * src = additional_data;

  if(max > buf+buf_len) return 0;
  while(p < max) *p++ = *src++;
  return offset+additional_data_len;
}

unsigned int secvest_prepare_radio_datagram(uint8_t * buf, unsigned int buf_len,
					    const uint8_t * payload, unsigned int payload_len) {
  unsigned int offset = 0;
  offset = secvest_add_bytes(buf, buf_len, offset, preamble, sizeof(preamble));
  offset = secvest_add_bytes(buf, buf_len, offset, sync_word, sizeof(sync_word));

  offset = secvest_add_bytes(buf, buf_len, offset, payload, payload_len);
  offset = secvest_add_bytes(buf, buf_len, offset, sync_word, sizeof(sync_word));

  offset = secvest_add_bytes(buf, buf_len, offset, payload, payload_len);
  offset = secvest_add_bytes(buf, buf_len, offset, sync_word, sizeof(sync_word));

  return offset;
}


unsigned int secvest_encode_diffman(uint8_t * buf, unsigned int buf_len,
				    uint8_t * payload, unsigned int payload_len) {

  uint8_t *s = payload, *d = buf;
  int i;
  int state = 0;

  while(d < buf + buf_len && s < payload + payload_len) {
    uint16_t dst = 0;
    uint8_t l = 0;

    for(i = 7; i >= 0; i--) {
      int bit = *s & (1 << i) ? 1 : 0;


      if(bit) {
	l = state == 0 ? 0b01 : 0b10;
      }
      else {
	l = state == 0 ? 0b00 : 0b11;
	state = state == 0 ? 1 : 0;
      }

      dst <<= 2;
      dst |= l;
    }

    *d++ = dst >> 8;
    *d++ = dst & 0xff;
    s++;
  }

  return payload_len * 2;
}


/**
 * Calculates the CRC for a payload.
 */
uint16_t secvest_calc_crc(uint8_t * buf, unsigned int len) {
  uint16_t crc = 0;

  uint32_t i, k;
  int polynomial = 0x1021;

  for(k = 0; k < len; k++ ) {

    for(i = 0; i < 8; i++ ) {
      int bit = ((buf[k] >> (7-i) & 1) == 1);
      int c15 = ((crc >> 15    & 1) == 1);
      crc <<= 1;
      if (c15 ^ bit) crc ^= polynomial;
    }
  }

  return crc;
}

/*
void print_bits(uint8_t * buf, unsigned int buf_len) {
  uint8_t *p = buf;
  int i;

  while(p < buf + buf_len) {
    for(i = 7; i >= 0; i--) {
      int bit = *p & (1 << i);
      printf("%d", bit ? 1 : 0);
    }
    p++;
    //printf(" ");
  }
  printf("\n");
}

*/

unsigned int secvest_prepare_datagram(uint8_t * out_buf, unsigned int out_buf_len,
				      uint8_t * payload, unsigned int payload_len) {

  uint8_t encoded_buf[100], tmp[100];

  // copy payload to a temp buffer
  unsigned int offset = secvest_add_bytes(tmp, sizeof(tmp), 0, payload, payload_len);

  // calculate the crc
  uint16_t crc = secvest_calc_crc(payload, payload_len);

  // append crc to temp buffer
  tmp[offset] = crc >> 8;
  tmp[offset+1] = crc & 0xff;

  // encode as differential manchester
  unsigned int l = secvest_encode_diffman(encoded_buf, sizeof(encoded_buf), tmp, payload_len+2);

  // construct a secvest datagram
  return secvest_prepare_radio_datagram(out_buf, sizeof(out_buf), encoded_buf, l);
}


unsigned int secvest_tx_sv2(uint8_t * buf, unsigned int len) {
	uint8_t out_buf[100];
	return secvest_prepare_datagram(out_buf, sizeof(out_buf), buf, len);
}
