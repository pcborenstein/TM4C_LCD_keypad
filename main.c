
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"

void screenCommand(uint8_t cmd);
void toggleE(void);
void smallDelay(void);
uint8_t pollKeypad(void);
void printKey(uint8_t key);
void printChar(char symbol);
void myTimerISR(void);

/**
 * main.c
 */
int main(void)
{
    //
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    // from the datasheet: "When using the PLL, the VCO frequency of 400 MHz is predivided by 2 before the divisor is applied."
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    //D0-7 on PB0-7
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, 0xff);
    //E on D2
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2);
    //R/W on D1
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_1);
    //RS on D0
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0);
    //LED on PF4
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4);
    //keypad outputs
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, (GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |  GPIO_PIN_4));
    //keypad inputs
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7));
    //put pull downs on keypad inputs
    GPIOPadConfigSet(GPIO_PORTA_BASE, (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7), GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

    //set up a timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));


    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    //80 thousand ticks creates a ISR period of 16ms
    //the screen takes >15ms to wake.
    TimerLoadSet(TIMER0_BASE, TIMER_A, 80000000);
    uint32_t intStatus = TimerIntStatus(TIMER0_BASE, false);
    TimerIntClear(TIMER0_BASE, intStatus);
    TimerEnable(TIMER0_BASE, TIMER_A);

    //wait for the first flag
    while(TimerIntStatus(TIMER0_BASE, false) == 0);
    intStatus = TimerIntStatus(TIMER0_BASE, false);
    TimerIntClear(TIMER0_BASE, intStatus);

    //set a slower period of 0.5s
    TimerLoadSet(TIMER0_BASE, TIMER_A, 25000000);

    TimerIntRegister(TIMER0_BASE, TIMER_A, myTimerISR);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //set all outputs 0
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 0);
    GPIOPinWrite(GPIO_PORTD_BASE, 0xff, 0);
    GPIOPinWrite(GPIO_PORTE_BASE, 0xff, 0);

    screenCommand(0x30); //8-bit mode
    smallDelay();
    screenCommand(0x38); //8-bit mode, 2 lines
    smallDelay();
    screenCommand(0x01); //clear display
    smallDelay();
    screenCommand(0x0f); //blink cursor
    smallDelay();
    screenCommand(0x02); //return home
    //printMsg();
    uint8_t keyPress = 0;
    uint8_t keyPressOld = 0;
    while(1){
        keyPress = pollKeypad();
        if(keyPress != keyPressOld){
            keyPressOld = keyPress;
            if(keyPress != 0)
                printKey(keyPress);
        }
    }

	return 0;
}

void screenCommand(uint8_t cmd){
    //R/W and RS are low
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0);
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, 0);

    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, cmd);
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 0);
}

void myTimerISR(void){

    uint32_t intStatus = TimerIntStatus(TIMER0_BASE, false);

    uint8_t pinState = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
    pinState ^= 0xff;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, pinState);

    TimerIntClear(TIMER0_BASE, intStatus);

}


uint8_t pollKeypad(void){
    uint8_t i, j;
    uint8_t key = 0;
    uint8_t keyFound = 0;
    for(i = 1; i < 5; i++){
        GPIOPinWrite(GPIO_PORTE_BASE, (GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |  GPIO_PIN_4), (1 << i));
        for(j = 7; j > 3; j--){
            if(keyFound == 0)
                key++;
            if(GPIOPinRead(GPIO_PORTA_BASE, (1 << j)) != 0){
                //two keys pressed will not return accurate results
                if(keyFound == 1)
                    return 0;
                keyFound = 1;
            }
        }
    }
    GPIOPinWrite(GPIO_PORTE_BASE, (GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |  GPIO_PIN_4), 0);
    if(keyFound == 1)
        return key;
    else
        return 0;
}

void printKey(uint8_t key){
    switch(key){
    case 1:
        printChar('1');
        break;
    case 2:
        printChar('4');
        break;
    case 3:
        printChar('7');
        break;
    case 4:
        printChar('*');
        break;
    case 5:
        printChar('2');
        break;
    case 6:
        printChar('5');
        break;
    case 7:
        printChar('8');
        break;
    case 8:
        printChar('0');
        break;
    case 9:
        printChar('3');
        break;
    case 10:
        printChar('6');
        break;
    case 11:
        printChar('9');
        break;
    case 12:
        printChar('#');
        break;
    case 13:
        printChar('A');
        break;
    case 14:
        printChar('B');
        break;
    case 15:
        printChar('C');
        break;
    case 16:
        printChar('D');
        break;
    default:
        //shouldn't reach here
        break;
    }
}

void printChar(char symbol){
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, 0);
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, symbol);
    toggleE();
    smallDelay();
}

void printMsg(void){
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'h');
    toggleE();
    smallDelay();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'e');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'l');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'l');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'o');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, ' ');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'a');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'b');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'c');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 'd');
    toggleE();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 0);
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0);
}


void toggleE(void){
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);

    smallDelay();

    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0);
}

void smallDelay(void){

    volatile uint32_t ui32Loop;

    for(ui32Loop = 0; ui32Loop < 20000; ui32Loop++)
    {
    }
}
