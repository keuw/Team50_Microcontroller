/*
 * File:   main.c
 * Author: True Administrator
 *
 * Created on July 18, 2016, 12:11 PM
 */


#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"

#define __delay_1s() for(char i=0;i<100;i++){__delay_ms(10);}
#define __lcd_newline() lcdInst(0b11000000);
#define __lcd_clear() lcdInst(0x01);
#define __lcd_home() lcdInst(0b10000000);

void servoRotate0();
void servoRotate90();
void servoRotate180();

const char keys[] = "Z23A456B789C*0#D"; 

void Motors(void) {
    TRISC = 0x00;
    TRISD = 0x00;   //All output mode
    TRISB = 0xFF;   //All input mode
    LATB = 0x00; 
    LATC = 0x00;
    ADCON0 = 0x00;  //Disable ADC
    ADCON1 = 0xFF;  //Set PORTB to be digital instead of analog default  
    initLCD();
    
    while(1){

           servoRotate0(); //0 Degree
           __delay_ms(2000);
           servoRotate90(); //90 Degree
           __delay_ms(2000);
           servoRotate180(); //180 Degree
           __delay_ms(2000);

    }
    
    return;
}

void servoRotate0() //0 Degree
{
  unsigned int i;
  for(i=0;i<50;i++)
  {
    PORTEbits.RE1 = 1;
    __delay_us(800);
    PORTEbits.RE1 = 0;
    __delay_us(19200);
  }
}

void servoRotate90() //90 Degree
{
  unsigned int i;
  for(i=0;i<50;i++)
  {
    PORTEbits.RE1 = 1;
    __delay_us(1500);
    PORTEbits.RE1 = 0;
    __delay_us(18500);
  }
}

void servoRotate180() //180 Degree
{
  unsigned int i;
  for(i=0;i<50;i++)
  {
    PORTEbits.RE1 = 1;
    __delay_us(2200);
    PORTEbits.RE1 = 0;
    __delay_us(17800);
  }
}
