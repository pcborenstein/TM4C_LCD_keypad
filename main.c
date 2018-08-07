
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

void toggleE(void);
void blinkCursor(void);
/**
 * main.c
 */
int main(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    //D0-7 on PB0-7
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, 0xff);
    //E on D2
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2);
    //R/W on D1
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_1);
    //RS on D0
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0);

    //set all outputs 0
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 0);
    GPIOPinWrite(GPIO_PORTD_BASE, 0xff, 0);

    blinkCursor();
    while(1);

	return 0;
}

void blinkCursor(void){
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 0);
}

void toggleE(void){
    volatile uint32_t ui32Loop;

    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);

    for(ui32Loop = 0; ui32Loop < 2000; ui32Loop++)
    {
    }
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0);
}
