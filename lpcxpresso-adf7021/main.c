
void send_rx_msg(const uint8_t *  buf, unsigned int len) {
	static char to_hex[] = "0123456789abcdef";
	const uint8_t * p = buf;

	VCOM_puts((uint8_t *)"RX ", 3);
	while(p < buf + len) {
		uint8_t b = *p++;
		VCOM_putchar(to_hex[b >> 4]);
		VCOM_putchar(to_hex[b & 0xf]);
	}
	VCOM_puts((uint8_t *)"\r\n", 2);
}


void send_ok_msg() {
	VCOM_puts((uint8_t *)"OK\r\n", 4);
}

void send_error_msg() {
	VCOM_puts((uint8_t *)"Error\r\n", 7);
}


uint8_t * get_cmd_param(const uint8_t * const buf, unsigned int len, unsigned int offset) {
	uint8_t * p = buf + offset;
	while(p < buf + len) {
		if(*p == ' ') p++;
		else return p;
	}
	return NULL;
}

const char cmd_tx_raw[] = "tx_raw";
const char cmd_tx_sv2[] = "tx_sv2";
const char cmd_set_rx_max_length[] = "set_rx_max_length";
const char cmd_set_tx_test_mode[] = "set_tx_test_mode";


void process_command(const uint8_t * const buf, unsigned int len) {

	int a, b;

	if((a = !strncmp((const char * const)buf, cmd_tx_raw, sizeof(cmd_tx_raw)-1 )) ||
			(b = !strncmp((const char * const)buf, cmd_tx_sv2, sizeof(cmd_tx_sv2)-1 )) ) {
		uint8_t * param_start = get_cmd_param(buf, len, sizeof(cmd_tx_raw));
		if(param_start != NULL) {
			uint8_t msg_buf[100];
			unsigned int param_len = len - (param_start - buf);
			unsigned int l = secvest_load_hexstr(msg_buf, sizeof(msg_buf), param_start, param_len);
			if(l > 0) {
				if(adf7021_in_rx_mode()) adf7021_mode_tx();
				unsigned int l2 = 0;
				if(a) l2 = secvest_tx_raw(msg_buf, l);
				else l2 = secvest_tx_sv2(msg_buf, l);

				if(l2 == l) send_ok_msg();
				else send_error_msg();
			}
			else send_error_msg();
		}
		else send_error_msg();
	}
	else if(!strncmp((const char * const)buf, "set_tx_mode", len)) {
		adf7021_mode_tx();
		send_ok_msg();
	}
	else if(!strncmp((const char * const)buf, cmd_set_tx_test_mode, sizeof(cmd_set_tx_test_mode) - 1 )) {
		uint8_t * param_start = get_cmd_param(buf, len, sizeof(cmd_set_tx_test_mode));
		if(param_start != NULL) {
			int mode = atoi(param_start);

			if(mode == 0) adf7021_mode_rx();
			else if(mode > 0 && adf7021_in_rx_mode()) adf7021_mode_tx();

			if(adf7021_set_test_mode(mode)) send_ok_msg();
			else send_error_msg();
		}
		else {
			send_error_msg();
		}
	}
	else if(!strncmp((const char * const)buf, "set_rx_mode", len)) {
		adf7021_mode_rx();
		send_ok_msg();
	}
	else if(!strncmp((const char * const)buf, cmd_set_rx_max_length, sizeof(cmd_set_rx_max_length) - 1)) {

		int l = atoi((const char *)(buf + sizeof(cmd_set_rx_max_length) - 1));
		if(l > 0) {
			set_rx_max_length(l);
			send_ok_msg();
		}
		else {
			send_error_msg();
		}
	}
	else if(!strncmp((const char * const)buf, "help", 4)) {
		char msg[] =
				"+ Implemented commands:\r\n"
				"+ set_rx_mode              - enable receiving\r\n"
				"+ set_tx_mode              - switch to transmitter mode\r\n"
				"+ tx_raw <hexstr>  		- transmit data as given in the hexstring\r\n"
				"+ tx_sv2 <hexstr>  		- transmit data, treat hex string as SV2 payload\r\n"
				"+ set_tx_test_mode <mode>  - enable test transmissions (0: disable test and rx)\r\n"
				"+ set_rx_max_length <n>    - number of bytes to receive\r\n"
				"+ help                     - this message\r\n"
				"+\r\n"
				;
		VCOM_puts((uint8_t *)msg, sizeof(msg)-1);
	}
	else {
		char msg[] = "Error: unknown command\r\n";
		VCOM_puts((uint8_t *)msg, sizeof(msg)-1);
	}
}


/*************************************************************************
	main
	====
**************************************************************************/
int main(void)
{

	int c;
	uint8_t input_buf[1024];
	uint8_t * input_buf_ptr = input_buf;
	
	setup();
/*	while(1) {
		process_command("tx_raw deadbeef", 15);
	}
*/
	printf("Initialising USB stack\n");

	// initialise stack
	USBInit();

	// register descriptors
	USBRegisterDescriptors(abDescriptors);

	// register class request handler
	USBRegisterRequestHandler(REQTYPE_TYPE_CLASS, HandleClassRequest, abClassReqData);

	// register endpoint handlers
	USBHwRegisterEPIntHandler(INT_IN_EP, NULL);
	USBHwRegisterEPIntHandler(BULK_IN_EP, BulkIn);
	USBHwRegisterEPIntHandler(BULK_OUT_EP, BulkOut);
	
	// register frame handler
	USBHwRegisterFrameHandler(USBFrameHandler);

	// enable bulk-in interrupts on NAKs
	USBHwNakIntEnable(INACK_BI);

	// initialise VCOM
	VCOM_init();
	printf("Starting USB communication\n");

/* CodeRed - comment out original interrupt setup code	
	// set up USB interrupt
	VICIntSelect &= ~(1<<22);               // select IRQ for USB
	VICIntEnable |= (1<<22);

	(*(&VICVectCntl0+INT_VECT_NUM)) = 0x20 | 22; // choose highest priority ISR slot 	
	(*(&VICVectAddr0+INT_VECT_NUM)) = (int)USBIntHandler;
	
	enableIRQ();
*/	

// CodeRed - add in interrupt setup code for RDB1768

#ifndef POLLED_USBSERIAL
	//enable_USB_interrupts();
	NVIC_EnableIRQ(USB_IRQn); 
	
#endif
		
	// connect to bus
		
	printf("Connecting to USB bus\n");
	USBHwConnect(TRUE);
	
	// echo any character received (do USB stuff in interrupt)
	while (1) {
		
		// check for input data via USB
		if(fifo_avail(&rxfifo)) {
			c = VCOM_getchar();
			if ((c == '\r') || (c == '\n') || (c == '\0')) {
				if(input_buf_ptr > input_buf)
					process_command(input_buf, input_buf_ptr - input_buf /* wo the null byte */);
				input_buf_ptr = input_buf;
			}
			else {
				if(input_buf_ptr + 1 < input_buf + sizeof(input_buf))
					*input_buf_ptr++ = c;
					*input_buf_ptr = '\0'; // strictly not neccessary, because we usually pass a length parameter
			}
		}

		// check for input messages via adf7021
		if(adf7021_in_rx_mode()) {
			radio_message_type * m = get_next_radio_msg();
			if(m != NULL) {
				//printf("have message\n");
				send_rx_msg(m->data, m->length);
				m->length = 0;
			}
		}


	}

	return 0;
}

