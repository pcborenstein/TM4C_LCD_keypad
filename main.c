
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"

#include "keypad.h"

void screenCommand(uint8_t cmd);
void toggleE(void);
void delay100us(void);
uint8_t pollKeypad(void);
void printKey(uint8_t key);
void printChar(char symbol);
void myTimerISR(void);
void printMsg(char * msg);
void setFrequency(uint16_t frq);
void setFourBitMode(void);


#define LCD_E_PIN   GPIO_PIN_2
#define LCD_RW_PIN  GPIO_PIN_1
#define LCD_RS_PIN  GPIO_PIN_0


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
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, 0xf0);
    //E on D2
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, LCD_E_PIN);
    //R/W on D1
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, LCD_RW_PIN);
    //RS on D0
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, LCD_RS_PIN);
    //LED on PF4
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4);

    keypadInit();

    //set up a timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));


    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    //80 thousand ticks creates a ISR period of 16ms
    //the screen takes >15ms to wake.
    TimerLoadSet(TIMER0_BASE, TIMER_A, 80000);
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

    //wait 40ms
    volatile int wait;
    for(wait = 0; wait< 400; wait++){
        delay100us();
    }
    setFourBitMode();
    screenCommand(0x28);
    screenCommand(0x01); //clear display returns home every other on error, erases F and moves cursor to colin
    //wait 2ms
    for(wait = 0; wait< 20; wait++){
        delay100us();
    }
    screenCommand(0x0f); //blink cursor
    screenCommand(0x02); //return home
    //wait 2ms
    for(wait = 0; wait< 20; wait++){
        delay100us();
    }
    printMsg("ENTER FREQUENCY:__Hz");
    screenCommand(0xC0); //move to second line
    printMsg("CURRENT: 01Hz");
    //move to input
    screenCommand(0x80 | 0x10);
    uint8_t keyPress = 0;
    uint8_t keyPressOld = 0;
    uint16_t frq = 0;
    uint8_t digit = 1;
    char keyChar;
    char keyStr[4];
    while(1){
        keyPress = pollKeypad();
        //make sure we get a real value
        if(keyPress > KEY_PAD_BUTTONS)
            continue;
        if(keyPress != keyPressOld){
            keyPressOld = keyPress;
            //only print digits
            if(keyPress != 0){
                keyChar = keyToCharArray[keyPress-1];
                if((keyChar >= '0') && (keyChar <= '9')){
                    if(digit == 2)
                        frq += (keyChar - '0');
                    if(digit == 1)
                        frq += 10 * (keyChar - '0');
                    keyStr[digit - 1] = keyChar;
                    printChar(keyChar);
                    digit++;
                    if(digit > 2){
                        setFrequency(frq);
                        screenCommand(0x80 | 0x49);
                        printMsg(keyStr);
                        //move to input
                        screenCommand(0x80 | 0x10);
                        printMsg("__");
                        screenCommand(0x80 | 0x10);
                        digit = 1;
                        frq = 0;
                    }
                }
            }
            //wait 10ms
            for(wait = 0; wait< 100; wait++){
                delay100us();
            }
        }
    }

	return 0;
}

void screenCommand(uint8_t cmd){
    uint8_t lowerFourBits = (cmd << 4) & 0xf0;
    cmd &= 0xf0;

    //R/W and RS are low
    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RS_PIN, 0);
    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RW_PIN, 0);

    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, cmd);
    toggleE();
    delay100us();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, lowerFourBits);
    toggleE();
    delay100us();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 0);
}

void setFourBitMode(void){
    //R/W and RS are low
    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RS_PIN, 0);
    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RW_PIN, 0);

    //The data sheet has a flow chat
    //shows setting 8 bit mode
    //then setting 4 bit mode twice

    //DL high = 8-bit low = 4-bit
    //F must be low when N is 2 lines

    //example code shows 5ms delays between setting 8-bit mode
    //8-bit
    volatile int wait;
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, 0x30);
    toggleE();
    //wait 5ms
    for(wait = 0; wait< 20; wait++){
        delay100us();
    }
    toggleE();
    //wait 5ms
    for(wait = 0; wait< 20; wait++){
        delay100us();
    }
    toggleE();
    //wait 5ms
    for(wait = 0; wait< 20; wait++){
        delay100us();
    }


    //4-bit
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, 0x20);
    toggleE();
    delay100us();
    /*
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, 0x80);
    toggleE();
    delay100us();
    //4-bit x2
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, 0x20);
    toggleE();
    delay100us();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, 0x80);
    toggleE();
    delay100us();

    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, 0);
    */
}

void myTimerISR(void){

    uint32_t intStatus = TimerIntStatus(TIMER0_BASE, false);

    uint8_t pinState = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
    pinState ^= 0xff;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, pinState);

    TimerIntClear(TIMER0_BASE, intStatus);

}


void printKey(uint8_t key){
    uint8_t index = key - 1;
    if(index < KEY_PAD_BUTTONS)
        printChar(keyToCharArray[index]);
}


void printChar(char symbol){
    uint8_t lowerFourBits = (symbol << 4) & 0xf0;
    symbol &= 0xf0;

    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RS_PIN, LCD_RS_PIN);
    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RW_PIN, 0);
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, symbol);
    toggleE();
    delay100us();
    GPIOPinWrite(GPIO_PORTB_BASE, 0xf0, lowerFourBits);
    toggleE();
    delay100us();
}
/*
void printChar(char symbol){
    uint8_t lowerFourBits = (symbol << 4) & 0x0f;
    symbol &= 0xf0;

    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RS_PIN, LCD_RS_PIN);
    GPIOPinWrite(GPIO_PORTD_BASE, LCD_RW_PIN, 0);
    GPIOPinWrite(GPIO_PORTB_BASE, 0xff, symbol);
    toggleE();
    smallDelay();
}
*/

void printMsg(char * msg){
    int i, j;
    i = strlen(msg);
    for(j = 0;j < i; j++)
        printChar(msg[j]);
}


void toggleE(void){
    GPIOPinWrite(GPIO_PORTD_BASE, LCD_E_PIN, LCD_E_PIN);

    delay100us();

    GPIOPinWrite(GPIO_PORTD_BASE, LCD_E_PIN, 0);
}

void delay100us(void){

    volatile uint32_t ui32Loop;

    for(ui32Loop = 0; ui32Loop < 420; ui32Loop++);
}


void setFrequency(uint16_t frq){
    uint64_t ticks;
    ticks = (uint64_t) 25000000 / frq;
    TimerLoadSet(TIMER0_BASE, TIMER_A, ticks);
}
