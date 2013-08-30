/*
 * globals.h
 *
 *  Created on: Apr 6, 2012
 *      Author: martin
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#define p0_high(pin) (LPC_GPIO0->FIOSET = (1 << pin))
#define p0_low(pin) (LPC_GPIO0->FIOCLR = (1 << pin))
#define p0_input(pin) (LPC_GPIO0->FIODIR &= ~(1 << pin))
#define p0_output(pin) (LPC_GPIO0->FIODIR |= (1 << pin))
#define p0_read(pin) ( (LPC_GPIO0->FIOPIN & (1 << pin)) ? 1 : 0)

#define p1_high(pin) (LPC_GPIO1->FIOSET = (1 << pin))
#define p1_low(pin) (LPC_GPIO1->FIOCLR = (1 << pin))
#define p1_input(pin) (LPC_GPIO1->FIODIR &= ~(1 << pin))
#define p1_output(pin) (LPC_GPIO1->FIODIR |= (1 << pin))

#define p2_high(pin) (LPC_GPIO2->FIOSET = (1 << pin))
#define p2_low(pin) (LPC_GPIO2->FIOCLR = (1 << pin))
#define p2_input(pin) (LPC_GPIO2->FIODIR &= ~(1 << pin))
#define p2_output(pin) (LPC_GPIO2->FIODIR |= (1 << pin))

#define p3_high(pin) (LPC_GPIO3->FIOSET = (1 << pin))
#define p3_low(pin) (LPC_GPIO3->FIOCLR = (1 << pin))
#define p3_input(pin) (LPC_GPIO3->FIODIR &= ~(1 << pin))
#define p3_output(pin) (LPC_GPIO3->FIODIR |= (1 << pin))

#define p4_high(pin) (LPC_GPIO4->FIOSET = (1 << pin))
#define p4_low(pin) (LPC_GPIO4->FIOCLR = (1 << pin))
#define p4_input(pin) (LPC_GPIO4->FIODIR &= ~(1 << pin))
#define p4_output(pin) (LPC_GPIO4->FIODIR |= (1 << pin))

#define MODE_RECV

#endif /* GLOBALS_H_ */
