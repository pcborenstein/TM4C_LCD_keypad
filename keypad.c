/*
 * keypad.c
 *
 *  Created on: Dec 22, 2021
 *      Author: peter
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

#define COLUMN_BASE GPIO_PORTA_BASE
#define COLUMN_1    GPIO_PIN_6
#define COLUMN_2    GPIO_PIN_7
#define COLUMN_3    GPIO_PIN_5
#define COLUMN_ALL  (COLUMN_1 | COLUMN_2 | COLUMN_3)

#define ROW_BASE    GPIO_PORTE_BASE
#define ROW_1       GPIO_PIN_5
#define ROW_2       GPIO_PIN_2
#define ROW_3       GPIO_PIN_3
#define ROW_4       GPIO_PIN_1
#define ROW_ALL     (ROW_1 | ROW_2 | ROW_3 | ROW_4)


uint8_t pollKeypad(void){
    uint8_t column, row;
    uint8_t key = 0;
    uint8_t keyFound = 0;
    uint8_t rowPin = 0;
    for(column = 1; column < 4; column++){
        GPIOPinWrite(COLUMN_BASE, COLUMN_ALL, 0);
        switch(column){
        case 1:
            GPIOPinWrite(COLUMN_BASE, COLUMN_1, 0xff);
            break;
        case 2:
            GPIOPinWrite(COLUMN_BASE, COLUMN_2, 0xff);
            break;
        case 3:
            GPIOPinWrite(COLUMN_BASE, COLUMN_3, 0xff);
            break;
        default:
            break;
        }

        for(row = 1; row < 5; row++){
            switch(row){
            case 1:
                rowPin = ROW_1;
                break;
            case 2:
                rowPin = ROW_2;
                break;
            case 3:
                rowPin = ROW_3;
                break;
            case 4:
                rowPin = ROW_4;
                break;
            default:
                break;
            }
            if(keyFound == 0)
                key++;
            if(GPIOPinRead(ROW_BASE, rowPin) != 0){
                //two keys pressed will not return accurate results
                if(keyFound == 1)
                    return 0;
                keyFound = 1;
            }
        }
    }
    GPIOPinWrite(COLUMN_BASE, COLUMN_ALL, 0);
    if(keyFound == 1)
        return key;
    else
        return 0;
}

void keypadInit(void){


    //keypad outputs
    GPIOPinTypeGPIOOutput(COLUMN_BASE, COLUMN_ALL);
    GPIOPinWrite(COLUMN_BASE, COLUMN_ALL, 0);
    //keypad inputs
    GPIOPinTypeGPIOInput(ROW_BASE, ROW_ALL);
    //put pull downs on keypad inputs
    GPIOPadConfigSet(ROW_BASE, ROW_ALL, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

}
