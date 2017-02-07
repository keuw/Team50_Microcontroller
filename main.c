/*
 * File:   main.c
 * Author: True Administrator
 *
 * Created on July 18, 2016, 12:11 PM
 */


#include <xc.h>
#include <stdio.h>
#include <math.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "macros.h"

void set_time(void);
void update_time(unsigned char[]);
void delay(int);
void init_operation(unsigned char[], unsigned char[]);
void runtime(unsigned char[], unsigned char[]);
void bottle_count(void);
void operation_end(void);
void date_time(unsigned char[]);

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
    TRISC = 0x00;
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
                operation_end();
                break;
            case OPERATION:                 //Check for '1' Press
                init_operation(start_time, time);   
                update_time(end_time);
                break;
            case DATETIME:                  //Check for '4' Press
                date_time(time);
                break;
            case BOTTLECOUNT:               //Check for '3' Press
                bottle_count();
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

void init_operation(unsigned char start_time[], unsigned char time[]){
    update_time(start_time);                    //Update Starting Time of Operation
    __lcd_clear();
    initLCD();
    while(PORTBbits.RB1 == 0 && keys[(PORTB & 0xF0)>>4] != '*'){
        update_time(time);                      
        __lcd_home();
        printf("Press * To Stop ");
        __lcd_newline();
        printf("Elapsed: %is      ", time_difference(time, start_time));
        __delay_ms(500);
    }
}

void runtime(unsigned char start_time[], unsigned char end_time[]){
    __lcd_home();
    printf("Total Operation ");
    __lcd_newline();
    printf("Time: %is         ", time_difference(end_time, start_time));
}

void bottle_count(void){
    while (bot_type != O){
        switch(bot_type){
            case TOTAL:
                __lcd_home();
                printf("Total Bottle    ");
                __lcd_newline();
                printf("Count: __       ");
                break;
            case A:
                __lcd_home();
                printf("YOP With Cap    ");
                __lcd_newline();
                printf("Count: __       ");
                break;
            case B:
                __lcd_home();
                printf("YOP With No Cap ");
                __lcd_newline();
                printf("Count: __       ");
                break;
            case C:
                __lcd_home();
                printf("ESKA With Cap   ");
                __lcd_newline();
                printf("Count: __       ");
                break;
            case D:
                __lcd_home();
                printf("ESKA With No Cap");
                __lcd_newline();
                printf("Count: __       ");
                break;
        }
        __delay_ms(100);
    }

}

void operation_end(void){
    __lcd_home();
    printf("Operation Done! ");
    __lcd_newline();
    printf("                ");
    curr_state = RUNTIME;
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