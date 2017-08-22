/*
 * RTC.c
 *
 * Created: 3/29/2016 3:01:34 PM
 * Author : Etiq Technologies
 */ 
#define F_CPU 16000000UL

#include <avr/io.h>
#include "uart.h"
#include "TWI.h"

struct time
{
   unsigned char time[3];
   unsigned char date[4];
   
}input_time,current_time;

unsigned char *day[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
unsigned char am;
/********************TWI SECTION********************************************/

/**************************************RTC section***********************************/

void uart_write_time()                                         //function to write the decoded time into the uart
{
	signed char i;
	for(i=2;i>=0;i--)                                         //writes sec,min,hr
	{
	write_uart((current_time.time[i]/10)+48);
	write_uart((current_time.time[i]%10)+48);
	if(i>0)
	write_uart(':');
	}
	
	write_uart_strg("   ");                                //checking the global am,pm flag and writes am or pm
	if(!am)
	write_uart_strg("pm");
	else if(am)
	write_uart_strg("am");
	
	write_uart_strg("   ");
	write_uart_strg(day[(current_time.date[0])-1]);        //writes day eg:mon,sun
	write_uart_strg("   ");
	
	for(i=1;i<=3;i++)                                       //writes the date,month ,year
	{
		write_uart((current_time.date[i]/10)+48);
		write_uart((current_time.date[i]%10)+48);
		if(i<3)
		write_uart(':');
	}
	write_uart_strg("\n\r");
}

void decode_date()                                            //decode date from BCD to DEC
{
	unsigned char i=0;
	while(i<4)
	{
		current_time.date[i]=((current_time.date[i]>>4)*10)+(current_time.date[i]&0x0F);
		i++;
	}
	uart_write_time();                          // fn call to write time and date(decoded)
}


void decode_time()                               //decodes the time from           
{
unsigned char i=0;
while(i<3)	
{
if(i<2)                                               //BCD-DEC for sec and min reg
current_time.time[i]=((current_time.time[i]>>4)*10)+(current_time.time[i]&0x0F);

else                  //hr reg
{
if(current_time.time[i]&(1<<6))                             //if 12 hr mode
{
if(current_time.time[i]&(1<<5))                            //if PM
am=0;                                                     //global flag to identfy am,pm mode
else
am=1;

current_time.time[i]=(((current_time.time[i]>>4)&0x01)*10)+(current_time.time[i]&0x0F);	  //BCD-DEC clear 12hr mode bit and am,pm bit   
}
else
current_time.time[i]=((current_time.time[i]>>4)*10)+(current_time.time[i]&0x0F);    //if 24 hr mode BCD-DEC hr
}
i++;
}
decode_date();	                  //fn call to decode the date
}


void RTC_write_mode()                                //access RTC in write mode with sec reg address pointed
{
	TWI_send(0xD0);                                 //slave address RTC
	TWI_send(0x00);
	
}

void RTC_write_pointer_read_mode()                      //access RTC with sec pointed in read mode
{
TWI_start();
RTC_write_mode();
TWI_start();
TWI_send(0xD1);	                                        //slave address with read mode
}



void write_hour()
{
unsigned char mode,am_pm;
unsigned char digit;
write_uart_strg("Select mode\n\r 1) 12hr \n\r 2) 24hr");               // mode selection
mode=read_uart();
switch(mode)
{
case '1':                                                             //12 hr mode write hr value
write_uart_strg("\n\rEnter hr");
digit=(read_uart()-48);
input_time.time[2]=digit<<4;
digit=(read_uart()-48);
input_time.time[2]|=digit;
input_time.time[2]&=0x4F;

write_uart_strg("\n\rEnter \n\r1.am\n\r 2.pm ");                    //select am or pm
am_pm=read_uart();

if(am_pm=='1')                                                      //if am selected clearing am bit and 12hr mode bit of hr reg value(bit 5&6)
input_time.time[2]|=0x40;
if(am_pm=='2')                                                     //if pm selected setting the pm bit and 12hr mode bit(bit 5&6)
input_time.time[2]|=0x60;

TWI_send(input_time.time[2]);                                     //write hr reg
break;	
 
case '2':                                                        //selected 24 hr mode
write_uart_strg("\n\rEnter hr");
digit=(read_uart()-48);                                          //read the hr value
input_time.time[2]=digit<<4;
digit=(read_uart()-48);
input_time.time[2]|=digit;
input_time.time[2]&=0x3F;                                        // clearing the 24hr mode bit(bit6)

TWI_send(input_time.time[2]);                                   //write hr register of RTC
break;	
}

	
}
void Write_time()                   //RTC time write
{
	char i=0;                       //variable for loop iteration
	unsigned char digit;            //variable to store the digits of sec and min value
	
	while(i<3)                      //loop to enter the sec,min,hr to input time[3]
	{
    if(i==2)
	{
	write_hour();                   //hr register is written in write_hr fn 
	i++;	
	}
	
	else                          //writes second and min reg
	{
	digit=(read_uart()-48);
	input_time.time[i]=digit<<4;
	digit=(read_uart()-48);
	input_time.time[i]|=digit;
	TWI_send(input_time.time[i]);
	i++;	
	}
	
	}
}


void Write_date()                        //RTC write date
{
	char i=0;
	unsigned char digit;
	while(i<4)
	{
	digit=(read_uart()-48);
	input_time.date[i]=digit<<4;
	digit=(read_uart()-48);
	input_time.date[i]|=digit;
	TWI_send(input_time.date[i]);
	i++;
	}
}

void Read_date()
{
	unsigned char i=0;
	while(i<4)
	{
		if(i==3)
		current_time.date[i]=TWI_read_nack();
		else
		current_time.date[i]=TWI_read_ack();
		i++;
	}
	decode_time();
}


void Read_time()               //reads the value in RTC reg
{
unsigned char i=0;
while(i<3)	
{
if(i==2)
current_time.time[i]=TWI_read_ack();
else
current_time.time[i]=TWI_read_ack();	
i++;
}
Read_date();
}



/*****************main function****************************************/

int main(void)
{
   TWI_init();
   uart_initialize();
      TWI_start();
   RTC_write_mode(); 
   write_uart_strg("Enter the time seconds & minutes (SS:MM)");
   Write_time();
   write_uart_strg("Enter the Day (01-07) & Date (DDMMYY)");
   Write_date();
   
   
   
    while (1) 
    {
	TWI_stop();
	RTC_write_pointer_read_mode();
	Read_time();	
    }
}

