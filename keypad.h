/*
 * keypad.h
 *
 *  Created on: Dec 22, 2021
 *      Author: peter
 */

#ifndef KEYPAD_H_
#define KEYPAD_H_

#define KEYPAD_12 TRUE

#if KEYPAD_16
#define KEY_PAD_BUTTONS 16
char keyToCharArray[KEY_PAD_BUTTONS] = {'1', '4', '7', '*', '2', '5', '8', '0', '3', '6', '9', '#', 'A', 'B', 'C', 'D'};
#endif

#if KEYPAD_12 == TRUE
#define KEY_PAD_BUTTONS 12
#define INVALID_KEY     KEY_PAD_BUTTONS
extern char keyToCharArray[KEY_PAD_BUTTONS];
#endif

extern uint8_t keyPress;

void keypadInit(void);
void enableKeyDetection(void);
void delay100us(void);


#endif /* KEYPAD_H_ */
