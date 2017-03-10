/*
 * File:   main.c
 * Author: True Administrator
 *
 * Created on July 18, 2016, 12:11 PM
 */


#include <xc.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "macros.h"
#include "eeprom_routines.h"

void set_time(void);
void update_time(unsigned char[]);
void delay(int);
void init_operation(unsigned char[], unsigned char[], unsigned char[], unsigned char[]);
void runtime(unsigned char[], unsigned char[]);
void bottle_count(unsigned char []);
void operation_end(unsigned char []);
void date_time(unsigned char[]);
void servo_rotate(int);
void stepper(int);
void stepper_rev(int);
void stepper_state(unsigned char[]);
void read_colorsensor1(unsigned char[], unsigned char[], unsigned char[], unsigned char[]);
void read_colorsensor2(unsigned char[], unsigned char[], unsigned char[], unsigned char[]);
void eeprom_readbye(uint16_t, uint8_t);
uint8_t eeprom_readbyte(uint16_t);

const char keys[] = "123A456B789C*0#D";
const char happynewyear[7] = {  0x45, //45 Seconds 
                            0x59, //59 Minutes
                            0x23, //24 hour mode, set to 23:00
                            0x07, //Saturday 
                            0x31, //31st
                            0x12, //December
                            0x16};//2016

enum state {                        //Lists the states for switch/case
    STANDBY,
    OPERATION,
    OPERATION_END,
    DATETIME,
    BOTTLECOUNT,
    RUNTIME
};
enum state curr_state;

enum bottle_count {                 //Bottle Count Menu
    TOTAL,
    A,
    B,
    C,
    D,
    O
};
enum bottle_count bot_type;

void main(void) {
    
    // <editor-fold defaultstate="collapsed" desc=" STARTUP SEQUENCE ">
    
    TRISA = 0xFF; // Set Port A as all input
    TRISB = 0xFF; 
    TRISC = 0x18; //Input for sda and scl
    TRISD = 0x00; //All output mode for LCD
    TRISE = 0x00;    

    LATA = 0x00;
    LATB = 0x00; 
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    
    ADCON0 = 0x00;  //Disable ADC
    ADCON1 = 0xFF;  //Set PORTB to be digital instead of analog default  
    
    INT1IE = 1;
    ei();           //Enable all interrupts
    
    nRBPU = 0;
    
    //</editor-fold>

    __lcd_clear();
    initLCD();
    unsigned char time[7];
    unsigned char start_time[7];
    unsigned char end_time[7];
    unsigned char bot_count[4];
    unsigned char step_state[2];
    
    
    I2C_Master_Init(10000); //Initialize I2C Master with 100KHz clock
    
    // set_time();
    __lcd_clear();
    initLCD();
    __lcd_home();
    
    printf("Press 1 to begin operation");   //Starting Message
    curr_state = STANDBY;
    bot_type = O;
    while (1){
        switch(curr_state){
            case OPERATION_END:             //Check for '*' Press
                operation_end(step_state);
                break;
            case OPERATION:                 //Check for '1' Press
                init_operation(start_time, time, bot_count, step_state);   
                update_time(end_time);
                break;
            case DATETIME:                  //Check for '4' Press
                date_time(time);
                break;
            case BOTTLECOUNT:               //Check for '3' Press
                bottle_count(bot_count);
                break;
            case RUNTIME:                   //Check for '2' Press
                runtime(start_time, end_time);
                break;
        }
        __delay_ms(100);
    }
    return;
}

void interrupt keypressed(void) {           //Interrupt Enable for Keys
    if(INT1IF){
        switch((PORTB & 0xF0) >> 4){
            case 0b1100:                    //Key Press *
                curr_state = OPERATION_END;
                bot_type = O;
                break;
            case 0b0000:                    //Key Press 1
                curr_state = OPERATION;
                bot_type = O;
                break;
            case 0b0001:                    //Key Press 2
                curr_state = RUNTIME;
                bot_type = O;
                break;
            case 0b0010:                    //Key Press 3
                curr_state = BOTTLECOUNT;
                bot_type = TOTAL;
                break;
            case 0b0011:                    //Key Press A
                bot_type = A;
                break;
            case 0b0111:                    //Key Press B
                bot_type = B;
                break;
            case 0b1011:                    //Key Press C
                bot_type = C;
                break;
            case 0b1111:                    //Key Press D
                bot_type = D;
                break;
            case 0b0100:                    //Key Press 4
                curr_state = DATETIME;
                bot_type = O;
                break;
            default:
                break;
        }
    }
    INT1IF = 0;     //Clear flag bit
    return;
}

void set_time(void){
    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b11010000); //7 bit RTC address + Write
    I2C_Master_Write(0x00); //Set memory pointer to seconds
    for(char i=0; i<7; i++){
        I2C_Master_Write(happynewyear[i]);
    }    
    I2C_Master_Stop(); //Stop condition
}

void update_time(unsigned char time[]){
    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b11010000); //7 bit RTC address + Write
    I2C_Master_Write(0x00); //Set memory pointer to seconds
    I2C_Master_Stop(); //Stop condition

    //Read Current Time
    I2C_Master_Start();
    I2C_Master_Write(0b11010001); //7 bit RTC address + Read
    for(unsigned char i=0;i<0x06;i++){
        time[i] = I2C_Master_Read(1);
    }
    time[6] = I2C_Master_Read(0);       //Final Read without ack
    I2C_Master_Stop();
}

void delay(int seconds) {
    for (int i = 0; i <= seconds; i ++) {
        __delay_1s();
    }
}

int dec_to_hex(int num) {                   //Convert decimal unsigned char to hexadecimal int
    int i = 0, quotient = num, temp, hexnum = 0;
 
    while (quotient != 0) {
        temp = quotient % 16;
        
        hexnum += temp*pow(10,i);
        
        quotient = quotient / 16;
        i += 1;
    }
    return hexnum;
}

int hex_to_dec(int hexadecimal) {  // Doesn't work hehe xD
    long long decimalNumber=0;
    char hexDigits[16] = {0x0, 0x1,0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
      0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
    int i, j, power=0, digit;  
     
    /* Converting hexadecimal number to decimal number */
    for(i = 0; i < 4; i++) {
        /*search currect character in hexDigits array */
        for(j=0; j<16; j++){
            if(((hexadecimal >> 4*i) | 0xF) == hexDigits[j]){
                decimalNumber += j*pow(16, power);
            }
        }
        power++;
    }  
    hexadecimal = decimalNumber;
}

int time_difference(unsigned char time1[], unsigned char time2[]) {
    int hr1, hr2, min1, min2, s1, s2;
    int d1, d2, d3;
    hr1 = time1[2]; hr2 = time2[2]; min1 = time1[1]; min2 = time2[1]; 
    s1 = time1[0]; s2 = time2[0];
    
    d1 = dec_to_hex(hr1) - dec_to_hex(hr2);
    d2 = dec_to_hex(min1) - dec_to_hex(min2);
    d3 = dec_to_hex(s1) - dec_to_hex(s2);
    return 3600*d1 + 60*d2 + d3;                //Returns difference of 2 time arrays
}

void init_operation(unsigned char start_time[], unsigned char time[], unsigned char bot_count[], unsigned char step_state[]){
    update_time(start_time);                    //Update Starting Time of Operation
    __lcd_clear();
    initLCD();
    step_state[0] = 1;
    step_state[1] = 1;
    
    unsigned char detection_time[7];
    update_time(detection_time);
    for (int i = 0; i<4; i++){
        bot_count[i] = 0;
    }
    
    unsigned char red[2];
    unsigned char green[2];
    unsigned char blue[2];
    unsigned char clear[2];
    
    PORTEbits.RE1 = 1;
    PORTEbits.RE0 = 1;
    
    while((curr_state != OPERATION_END) && (time_difference(time, detection_time) <20)){
        update_time(time);    
        /*
        __lcd_home();
        printf("Press * To Stop ");
        __lcd_newline();
        printf("Elapsed: %is      ", time_difference(time, start_time));
        __delay_ms(500);
        */
        read_colorsensor1(red, green, blue, clear);
        int r1 = (red[0]<<8) | red[1];               //Concatenate the high and low bits
        int g1 = (green[0]<<8) | green[1];
        int b1 = (blue[0]<<8) | blue[1];
        int c1 = (clear[0]<<8) | clear[1];
        read_colorsensor2(red, green, blue, clear);
        int r2 = (red[0]<<8) | red[1];               //Concatenate the high and low bits
        int g2 = (green[0]<<8) | green[1];
        int b2 = (blue[0]<<8) | blue[1];
        int c2 = (clear[0]<<8) | clear[1];
        
        int luminosity1 = (-0.32466*r2) + (1.57837*g2) + (-0.73191*b2);
        
        __lcd_home();
        printf("%u       %u        ", r2, g2);
        __lcd_newline();
        printf("%u       %u        ", b2, time_difference(time, detection_time));
        
        if (r2 > 4000 && b2 < 2000){
            PORTEbits.RE0 = 0;
            bot_count[0] ++;
            __delay_ms(500);
            PORTEbits.RE0 = 1;
            step_state[1] = 1;
            stepper_state(step_state);
            update_time(detection_time);
        }
        else if (r2 > 3000 && g2 > 3000 && b2 > 3000){
            PORTEbits.RE0 = 0;
            bot_count[1] ++;
            __delay_ms(500);
            PORTEbits.RE0 = 1;
            step_state[1] = 2;
            stepper_state(step_state);
            update_time(detection_time);
        }
        
        else if (b2 > 4000 && r2 < 2500){
            PORTEbits.RE0 = 0;
            bot_count[2] ++;
            __delay_ms(500);
            PORTEbits.RE0 = 1;
            step_state[1] = 3;
            stepper_state(step_state);
            update_time(detection_time);
        }
        
        else if (c2 > 2000 && c2 < 2500){
            PORTEbits.RE0 = 0;
            bot_count[3] ++;
            __delay_ms(500);
            PORTEbits.RE0 = 1;
            step_state[1] = 4;
            stepper_state(step_state);
            update_time(detection_time);
        }
            
        __delay_ms(500);
    }
    curr_state = OPERATION_END;
}

void runtime(unsigned char start_time[], unsigned char end_time[]){
    __lcd_home();
    printf("Total Operation ");
    __lcd_newline();
    printf("Time: %is         ", time_difference(end_time, start_time));
}

void bottle_count(unsigned char bot_count[]){
    while (bot_type != O){
        switch(bot_type){
            case TOTAL:
                __lcd_home();
                printf("Total Bottle    ");
                __lcd_newline();
                printf("Count: %i       ", (bot_count[0] + bot_count[1] + bot_count[2] + bot_count[3]));
                break;
            case A:
                __lcd_home();
                printf("YOP With Cap    ");
                __lcd_newline();
                printf("Count: %i       ", bot_count[0]);
                break;
            case B:
                __lcd_home();
                printf("YOP With No Cap ");
                __lcd_newline();
                printf("Count: %i       ", bot_count[1]);
                break;
            case C:
                __lcd_home();
                printf("ESKA With Cap   ");
                __lcd_newline();
                printf("Count: %i       ", bot_count[2]);
                break;
            case D:
                __lcd_home();
                printf("ESKA With No Cap");
                __lcd_newline();
                printf("Count: %i       "), bot_count[3];
                break;
        }
        __delay_ms(100);
    }

}

void operation_end(unsigned char step_state[]){
    PORTEbits.RE1 = 0;
    __lcd_home();
    printf("Operation Done! ");
    __lcd_newline();
    printf("                ");
    curr_state = RUNTIME;
    step_state[1] = 1;
    stepper_state(step_state);
    delay(1);
    return;
}

void date_time(unsigned char time[]){
    //Reset RTC memory pointer 
    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b11010000); //7 bit RTC address + Write
    I2C_Master_Write(0x00); //Set memory pointer to seconds
    I2C_Master_Stop(); //Stop condition

    //Read Current Time
    I2C_Master_Start();
    I2C_Master_Write(0b11010001); //7 bit RTC address + Read
    for(unsigned char i=0;i<0x06;i++){
        time[i] = I2C_Master_Read(1);
    }
    time[6] = I2C_Master_Read(0);       //Final Read without ack
    I2C_Master_Stop();

    //LCD Display
    __lcd_home();
    printf("Date: %02x/%02x/%02x  ", time[5],time[4],time[6]);    //Print date in MM/DD/YY
    __lcd_newline();
    printf("Time: %02x:%02x:%02x  ", time[2],time[1],time[0]);    //HH:MM:SS

    return;
}

void servo_rotate(int degree)
{
  unsigned int i;
  unsigned int pwm = degree*(70/90) + 80;
  for(i=0;i<50;i++)
  {
    PORTEbits.RE1 = 1;
    for (unsigned int j = 0; j<pwm; j++)
        __delay_us(10);
    
    PORTEbits.RE1 = 0;
    for (unsigned int j = 0; j<(2000 - pwm); j++)
        __delay_us(10);
  }
}

void stepper(int r){
    
    for (int i = 0; i < r; i++){ //128 is 90 degrees
        PORTCbits.RC0 = 1;
        PORTCbits.RC1 = 1;
        PORTCbits.RC2 = 0;
        PORTCbits.RC5 = 0;
        __delay_ms(5);
        PORTCbits.RC0 = 0;
        PORTCbits.RC1 = 1;
        PORTCbits.RC2 = 1;
        PORTCbits.RC5 = 0;
        __delay_ms(5);
        PORTCbits.RC0 = 0;
        PORTCbits.RC1 = 0;
        PORTCbits.RC2 = 1;
        PORTCbits.RC5 = 1;
        __delay_ms(5);
        PORTCbits.RC0 = 1;
        PORTCbits.RC1 = 0;
        PORTCbits.RC2 = 0;
        PORTCbits.RC5 = 1;
        __delay_ms(5);
    }
}

void stepper_rev(int r){
    
    for (int i = 0; i < r; i++){ //128 is 90 degrees
        PORTCbits.RC0 = 1;
        PORTCbits.RC1 = 0;
        PORTCbits.RC2 = 0;
        PORTCbits.RC5 = 1;
        __delay_ms(5);
        PORTCbits.RC0 = 0;
        PORTCbits.RC1 = 0;
        PORTCbits.RC2 = 1;
        PORTCbits.RC5 = 1;
        __delay_ms(5);
        PORTCbits.RC0 = 0;
        PORTCbits.RC1 = 1;
        PORTCbits.RC2 = 1;
        PORTCbits.RC5 = 0;
        __delay_ms(5);
        PORTCbits.RC0 = 1;
        PORTCbits.RC1 = 1;
        PORTCbits.RC2 = 0;
        PORTCbits.RC5 = 0;
        __delay_ms(5);
    }
}

void stepper_state(unsigned char step_state[]){
    int i = 0;
    int curr_state = step_state[0];
    switch(curr_state){
        case 1:             //Check for '1' 
            if (step_state[1] = 2) stepper(128);
            else if (step_state[1] = 3) stepper(256);
            else if (step_state[1] = 4) stepper_rev(128);
            step_state[0] = 1;
            break;
        case 2:             //Check for '2' 
            if (step_state[1] = 3) stepper(128);
            else if (step_state[1] = 4) stepper(256);
            else if (step_state[1] = 1) stepper_rev(128);
            step_state[0] = 2;
            break;
        case 3:             //Check for '3' 
            if (step_state[1] = 4) stepper(128);
            else if (step_state[1] = 1) stepper(256);
            else if (step_state[1] = 2) stepper_rev(128);
            step_state[0] = 3;
            break;
        case 4:             //Check for '4' 
            if (step_state[1] = 1) stepper(128);
            else if (step_state[1] = 2) stepper(256);
            else if (step_state[1] = 3) stepper_rev(128);
            step_state[0] = 4;
            break;
    }
}

void read_colorsensor1(unsigned char red[],unsigned char green[],unsigned char blue[],unsigned char clear[]){
    //Write to Multiplexer
    I2C_Master_Start();
    I2C_Master_Write(0b11100000);   //7bit address 0x70 + Write
    I2C_Master_Write(0b10000000);   //Write to cmdreg
    I2C_Master_Write(0b10000000);   //Enable control register 7 (Sensor 1)
    I2C_Master_Stop();
    
    //Write Start Condition
    I2C_Master_Start();
    I2C_Master_Write(0b01010010);   //7bit address 0x29 + Write
    I2C_Master_Write(0b10000000);   //Write to cmdreg + access enable reg
    I2C_Master_Write(0b00000011);   //Start RGBC and POWER 
    I2C_Master_Stop();
    
    //Colour data
    I2C_Master_Start();
    I2C_Master_Write(0b01010010);   //7bit address 0x29 + Write
    I2C_Master_Write(0b10110100);   //Write to cmdreg + access&increment clear low reg
    I2C_Master_Start();
    I2C_Master_Write(0b01010011);   //7bit address 0x29 + Read
    
    clear[1] = I2C_Master_Read(1);  //Read clear with acknowledge (low bit)
    clear[0] = I2C_Master_Read(1);  //High bit
    
    red[1] = I2C_Master_Read(1);    //Read red with acknowledge (low bit)
    red[0] = I2C_Master_Read(1);    //High bit
    
    green[1] = I2C_Master_Read(1);  //Read green red with acknowledge (low bit)
    green[0] = I2C_Master_Read(1);  //High bit
    
    blue[1] = I2C_Master_Read(1);   //Read blue red with acknowledge (low bit)
    blue[0] = I2C_Master_Read(0);   //High bit
    
    I2C_Master_Stop();              //Stop condition
    
}

void read_colorsensor2(unsigned char red[],unsigned char green[],unsigned char blue[],unsigned char clear[]){
    //Write to Multiplexer
    I2C_Master_Start();
    I2C_Master_Write(0b11100000);   //7bit address 0x70 + Write
    I2C_Master_Write(0b10000000);   //Write to cmdreg
    I2C_Master_Write(0b00000100);   //Enable control register 2 (Sensor 2)
    I2C_Master_Stop();
    
    //Write Start Condition
    I2C_Master_Start();
    I2C_Master_Write(0b01010010);   //7bit address 0x29 + Write
    I2C_Master_Write(0b10000000);   //Write to cmdreg + access enable reg
    I2C_Master_Write(0b00000011);   //Start RGBC and POWER 
    I2C_Master_Stop();
    
    //Colour data
    I2C_Master_Start();
    I2C_Master_Write(0b01010010);   //7bit address 0x29 + Write
    I2C_Master_Write(0b10110100);   //Write to cmdreg + access&increment clear low reg
    I2C_Master_Start();
    I2C_Master_Write(0b01010011);   //7bit address 0x29 + Read
    
    clear[1] = I2C_Master_Read(1);  //Read clear with acknowledge (low bit)
    clear[0] = I2C_Master_Read(1);  //High bit
    
    red[1] = I2C_Master_Read(1);    //Read red with acknowledge (low bit)
    red[0] = I2C_Master_Read(1);    //High bit
    
    green[1] = I2C_Master_Read(1);  //Read green red with acknowledge (low bit)
    green[0] = I2C_Master_Read(1);  //High bit
    
    blue[1] = I2C_Master_Read(1);   //Read blue red with acknowledge (low bit)
    blue[0] = I2C_Master_Read(0);   //High bit
    
    I2C_Master_Stop();              //Stop condition
}

uint8_t eeprom_readbyte(uint16_t address) {

    // Set address registers
    EEADRH = (uint8_t)(address >> 8);
    EEADR = (uint8_t)address;

    EECON1bits.EEPGD = 0;       // Select EEPROM Data Memory
    EECON1bits.CFGS = 0;        // Access flash/EEPROM NOT config. registers
    EECON1bits.RD = 1;          // Start a read cycle

    // A read should only take one cycle, and then the hardware will clear
    // the RD bit
    while(EECON1bits.RD == 1);

    return EEDATA;              // Return data
}

void eeprom_writebyte(uint16_t address, uint8_t data) {    
    // Set address registers
    EEADRH = (uint8_t)(address >> 8);
    EEADR = (uint8_t)address;

    EEDATA = data;          // Write data we want to write to SFR
    EECON1bits.EEPGD = 0;   // Select EEPROM data memory
    EECON1bits.CFGS = 0;    // Access flash/EEPROM NOT config. registers
    EECON1bits.WREN = 1;    // Enable writing of EEPROM (this is disabled again after the write completes)

    // The next three lines of code perform the required operations to
    // initiate a EEPROM write
    EECON2 = 0x55;          // Part of required sequence for write to internal EEPROM
    EECON2 = 0xAA;          // Part of required sequence for write to internal EEPROM
    EECON1bits.WR = 1;      // Part of required sequence for write to internal EEPROM

    // Loop until write operation is complete
    while(PIR2bits.EEIF == 0)
    {
        continue;   // Do nothing, are just waiting
    }

    PIR2bits.EEIF = 0;      //Clearing EEIF bit (this MUST be cleared in software after each write)
    EECON1bits.WREN = 0;    // Disable write (for safety, it is re-enabled next time a EEPROM write is performed)
}