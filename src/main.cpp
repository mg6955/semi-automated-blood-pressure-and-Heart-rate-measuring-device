/***************************************************************************************
*                                                                                      *
* @filename: main.cpp                                                                  *
* @details:  This is the file for Embedded Challenge " Pressure is on".                *
*            This file contains functions that does Oscillometric measurement of       *
*            Blood Pressure and heart rate.                                            *
*            Assumption: The indication of Blood Pressure thresholds                   *
*            such as "HIGH","NORMAL", "LOW" is with assumption that the measurement    *
*            is for Adults                                                             *
* @authors:   Monisha Gnanaprakasam - mg6955, Sung Kim - sk9623                        *
* @date:      05/16/2022                                                               *
*                                                                                      *
****************************************************************************************/
#include <mbed.h>
#include "drivers/LCD_DISCO_F429ZI.h"


//Slave address : 0x18 
#define ADDR_WRITE 0x30 // Write bit:0 -> Write Address = 0x18<<0 -> 0011 0000 ->0x30  
#define ADDR_READ 0x31  // Read bit:1  -> Read Address  = 0x18<<1 -> 0011 0001 ->0x31

//Pressure sensor measures from 0 to 300 mmHg
#define PMIN 0.0
#define PMAX 300.0

#define OMIN 419430.4   // 2.5% of 2^24
#define OMAX 3774873.6  // 22.5% of 2^24

// Pin Configurations
// LED to indicate the initialization is done
DigitalOut led1(LED1);

// I2C variables 
I2C i2c(PC_9,PA_8);    // SDA - PC9;SCL - PA8

char sensor_val[5];    // Buffer to get values from I2C-SDA
uint32_t buf_s[4];     // Buffer to extract and manipulate Sensor data
  
//LCD Constants
#define BACKGROUND 1
#define FOREGROUND 0
#define GRAPH_PADDING 5

// Timer objects
Timer t;               // Calculating elapse time 

//Output variables
float systole = 0.0;
float diastole =0.0;
uint32_t heart_rate =0;

//Arrays to store Pressure,Time and slope values
float arr_p[1000] ;
float arr_t[1000] ;
float arr_s[1000] ;

//Indices for arrays
int idx_max =0;
int sysidx = 0;
int diaidx = 0;
float hridx=0.0;

//Counter variable to traverse arrays
int ctr;

// Value to store old pressure value
float pressure_old;

// Slope Maximum Threshold 
float max_PositiveSlope=0.0; 

//Change in pressure value
double delP=0;

//Change in time value
unsigned long delT = 0;

//Flags
static int flaga =0;        // Indicate pressure measurement is in deflation phase
uint8_t printflag=0;        // Indicate End of measurement to print computed values
int skipFirstIteration = 0; // To avoid the slope change at 150 mmHg

//LCD variables
LCD_DISCO_F429ZI lcd;

//buffer for holding displayed text strings
char display_buf[5][60];

/*
Function Name : setup_background_layer 
Parameters    : void
return type   : void
Description   : Sets the background layer to be visible, transparent, and resets its colors to all black
*/

void setup_background_layer(){
  lcd.SelectLayer(BACKGROUND);
  lcd.Clear(LCD_COLOR_BLACK);
  lcd.SetBackColor(LCD_COLOR_BLACK);
  lcd.SetTextColor(LCD_COLOR_GREEN);
  lcd.SetLayerVisible(BACKGROUND,ENABLE);
  lcd.SetTransparency(BACKGROUND,0x7Fu);
}

/*
Function Name : setup_foreground_layer
Parameters    : void
return type   : void
Description   : Resets the foreground layer to all black
*/
void setup_foreground_layer(){
    lcd.SelectLayer(FOREGROUND);
    lcd.Clear(LCD_COLOR_BLACK);
    lcd.SetBackColor(LCD_COLOR_BLACK);
    lcd.SetTextColor(LCD_COLOR_LIGHTGREEN);
}

/*
Function Name : inflate
Parameters    : pressure - pressure value calculated from raw data
return type   : int - value to check if deflation can be started or not
Description   : Function checks the value of pressure and returns integer to set or ignore deflation
*/
int inflate(float pressure){
int ret =0;

  if(flaga ==1)
  {
    skipFirstIteration = 1;
    if(pressure >30)
      ret =1;
    else 
      printflag =1;
  }  
    
  if (pressure > 150 && flaga ==0)
  {
    printf("Pressure:%f\n",pressure);
    flaga =1;
    ret =1;
  }
  if(pressure < 150 && flaga ==0)
    ret =2;
  
  return ret;
  
}

/*
Function Name : calculatesys
Parameters    : void
return type   : void
Description   : Calculate systolic pressure by comparing slope values
*/
void calculatesys(){

  float minThres = 0.5 * max_PositiveSlope;
  float delslope_s = 0;
  float mindels = INT32_MAX; 

  for(uint16_t l=0; l < idx_max-1; l++)             // For every value of pressure stored, slope is compared to threshold value
    if((arr_s[l]>=0.0) && (arr_s[l] < minThres))    // Change in slope is the difference between threshold and array value
    {
      delslope_s = minThres - arr_s[l]; 
      if(delslope_s < mindels)                      // If slope is less than the minimum change, minimum change is reassigned and index is systole changed 
      {
        mindels=delslope_s;
        sysidx=l+1;
      }  
    }

  systole = arr_p[sysidx];                          // Systole value is the pressure value at the index obtained
  printf("SYSTOLE : %f \n",systole);

  snprintf(display_buf[1],60,"Systole:%f",systole); // Systole Value printed in LCD
  lcd.SelectLayer(FOREGROUND);   
  lcd.DisplayStringAt(0, LINE(5), (uint8_t *)display_buf[1],LEFT_MODE);
}

/*
Function Name : calculatedias
Parameters    : void
return type   : void
Description   : Calculate diastolic pressure by comparing slope values
*/
void calculatedias(){
  float minThres_d = 0.8 * max_PositiveSlope; 
  float delslope_d=0; 
  float mindels_d=INT32_MAX;

  for(uint16_t l=idx_max+1; l < ctr; l++)             // For every value of pressure, slope is compared to threshold value until a value lesser than slope is obtained  
    if((arr_s[l]>=0.0) && (arr_s[l] < minThres_d))    // Difference between  threshold value and slope reading
    {
      delslope_d = minThres_d - arr_s[l];
      if(delslope_d < mindels_d)                      // If slope is less than the minimum change, minimum change is reassigned and index is diastole changed 
      {
        mindels_d=delslope_d; 
        diaidx=l+1; 
      }
    }

  
  diastole = arr_p[diaidx];                           // Diastole value is the pressure value at the index obtained
  printf("DIASTOLE : %f \n",diastole);
  
  snprintf(display_buf[2],60,"Diastole:%f",diastole); // Systole Value printed in LCD
  lcd.SelectLayer(FOREGROUND);   
  lcd.DisplayStringAt(0, LINE(7), (uint8_t *)display_buf[2],LEFT_MODE);
}

/*
Function Name : calculateHR
Parameters    : void
return type   : void
Description   : Calculate heart rate by Number of samples/ second * 60
*/
void calculateHR(){

  for(uint16_t l=sysidx-1;l<diaidx;l++)                                  // The count of non-zero slopes are obtained
    if(arr_s[l]>0.0)
      hridx ++;

  // Hear rate = Samples/Time in minutes 
  heart_rate = (int)(60/(hridx/((arr_t[diaidx]- arr_t[sysidx])) +0.5)); // 0.5 for pressure sensor bias(to be removed if not needed)
  printf("Heart Rate:%d\n",heart_rate);               // Heart rate printed in LCD
  snprintf(display_buf[3],60,"Heart Rate:%d",heart_rate);               // Heart rate printed in LCD
  lcd.SelectLayer(FOREGROUND);   
  lcd.DisplayStringAt(0, LINE(9), (uint8_t *)display_buf[3],LEFT_MODE);
}

/*
Function Name : calculatePressure
Parameters    : raw - float value that gives the raw sensor data
return type   : void
Description   : The function calculates the actual pressure value in mmHg as per the datasheet
*/
void calculatePressure(float raw){
  float pressure = float((( (raw - OMIN) * (PMAX - PMIN)) / (OMAX - OMIN)) + PMIN); // Calculate pressure as per data sheet
  int deflate =  inflate(pressure);                                                 // Deflate gives the phase of the measurement -> Inflatin/Deflation
 
  // Inflation Phase
  if (deflate ==2)
    printf(" Inflating the Cuff : Pressure:%f\n",pressure);

  // Deflation Phase
  if (deflate ==1 && skipFirstIteration == 1)
  {
    float diff = pressure_old - pressure;                   // Pressure change
    
    // Time between 2 readings = 79 ms -> 4mmHg/79 = 0.05
    if(diff < 0.05)                                         // Slow deflation
      printf("Pace up! Releasing too Slow\n");
    else if(diff > 1.75)                                    // Fast Deflation
      printf("Slow down! Releasing too Fast\n");
    else                                                    // Nominal Deflation
      printf("Maintain the pace\n");

    int tim = t.read_ms();
    if (pressure <140)
    { 
      arr_p[ctr] = pressure;
      arr_t[ctr] = tim/1000;
      ctr++;
      for(uint16_t l1=1;l1<ctr;l1++) 
      {
        float delp=arr_p[l1]-arr_p[l1-1];                  // Calculate change in Pressure 
        float delt=(arr_t[l1]-arr_t[l1-1]);                // Calculate change in Time
        if(delt != 0 )                                     // Avoid divide by zero exception
          arr_s[l1-1]= (delp/delt); 

      }
    
      for(uint16_t l2=0; l2<ctr; l2++) 
        if(arr_s[l2] > max_PositiveSlope)                  // Obtain highest slope value and index
        {
          max_PositiveSlope = arr_s[l2];
          idx_max = l2+1; 
          
        }
    }       
  }
  t.stop();
  wait_us(50000);
  pressure_old = pressure;                                // Store the current pressure value for next cycle
  
}

/*
Function Name : readPressure
Parameters    : void
return type   : void
Description   : Reads the sensor value through I2C protocol
*/
void readPressure(){
  // Command as per data sheet
  char cmd[4];
  cmd[0] =0xAA;
  cmd[1] =0x00;
  cmd[2] =0x00;
  
  i2c.write(ADDR_WRITE,cmd,3);        // Bring sensor to operation mode
  wait_us(15000);                     // Ensure enough delay so that sensor is not BUSY
  i2c.read(ADDR_READ,sensor_val,4);   // Read status and data (32 bits) LSB: Status
  wait_us(5000);
         
  if(sensor_val[0]!=0)
  {
    // Sensor data 24 bit::  2nd byte - 23:16 bits, 3rd byte - 8:15 bits, 1st byte - 0:7 bits
    buf_s[0] = (uint32_t)sensor_val[1]<<16;
    buf_s[1] = (uint32_t)sensor_val[2]<<8;
    buf_s[2] = (uint32_t)sensor_val[3];  
    float raw = float (buf_s[0] | buf_s[1] | buf_s[2]); 
    calculatePressure(raw);
  }
}

/*
Function Name : main
Parameters    : void
return type   : int 
Description   : Driver function
*/
int main() {
  // Program Initialization - LED and LCD Indication
  snprintf(display_buf[0],60,"THE PRESSURE IS ON");
  lcd.SelectLayer(FOREGROUND);
  lcd.DisplayStringAt(0, LINE(1), (uint8_t *)display_buf[0],CENTER_MODE); 
  led1.write(1);
  wait_us(100000);
  led1.write(0);  
     
  while(1) {   
    t.start();
    readPressure();
    wait_us(100000);
    if(printflag ==1)
    {
      calculatesys(); 
      calculatedias();
      calculateHR();
      if(systole > 140 && diastole >90)
        printf(" HYPERTENSION  STAGE2 - Consult a Physician \n");
      else if(systole > 130 && diastole > 81 )
        printf(" HYPERTENSION  STAGE1 - Consult a Physician \n");
      else if(systole > 120)
        printf(" ELEVATED Blood Pressure - Relax!!! \n");
      else
        printf(" NORMAL\n");
      break;
    }
  }
  return 0;
}