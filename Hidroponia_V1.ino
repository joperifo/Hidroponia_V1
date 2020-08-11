#include <EEPROM.h>
#include <dht.h>
#include <GravityTDS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"


//Pins Define
#define TdsSensorPin A0
#define PhSensorPin A1          
#define DHT11_PIN 2
#define ONE_WIRE_BUS 3
#define WaterPump 4
#define Ph_Up_Valve 5
#define Nutrients_Valve 6
#define Water_Valve 7


//Global define
#define DHTTYPE DHT11   // DHT 11
#define VOLTAGE 5.00    //system voltage
#define PH_OFFSET 0        //zero drift voltage
#define PhArrayLenth  40    //PH sensor number of samples

#define I2C_ADDRESS 0x3C    //OLED Display
#define RST_PIN -1 // Reset pin # (or -1 if sharing Arduino reset pin)

SSD1306AsciiWire oled;
dht DHT;

GravityTDS SensorTDS;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature WaterTempSensor(&oneWire);

//Global Vars
int PhArray[PhArrayLenth];
int PhArrayIndex=0;
double PhValue_mV;
float PhValue;
float Ext_Humidity, Ext_Temperature;

float temperature = 25,tdsValue = 0;

uint32_t ticks, last_tick_20ms, last_tick_1000ms,timmer_1s, timmer_5s, timmer_1m, timmer_10m, timmer_30m, timmer_1h ;
float WaterTemp=0;



#pragma region Init Setup
void setup() {
  //Init Serial
  Serial.begin(115200);
  //OLED Display init
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Arial_bold_14);
  //Splash Screen
  oled.clear();
  oled.setCursor(15,0);
  oled.println("Hidroponia V1.0");
  oled.setCursor(0,10);
  oled.println("================");
  oled.setCursor(25,30);
  oled.println("Joao Fontes");
  oled.setCursor(0,40);
  oled.println("================");

  delay(3000);
  
  //TDS Sensor
  InitSensorTDS();
  //Splash Screen TDS Sensor
  oled.clear();
  oled.setCursor(15,0);
  oled.println("TDS Sensor");
  oled.setCursor(0,10);
  oled.println("================");
  oled.setCursor(50,30);
  oled.println("OK");
  oled.setCursor(0,40);
  oled.println("================");

  delay(1000);
  
  //Water Temperature Sensor
  WaterTempSensor.begin();

  //Splash Screen Water Temperature Sensor
  oled.clear();
  oled.setCursor(0,0);
  oled.println("Water Sensor");
  oled.setCursor(0,10);
  oled.println("================");
  oled.setCursor(50,30);
  oled.println("OK");
  oled.setCursor(0,40);
  oled.println("================");

  delay(1000);
  
  //DHT11 Test 
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT_LIB_VERSION);
  Serial.println(); 
 int chk = DHT.read11(DHT11_PIN);
  switch (chk)
  {
    case DHTLIB_OK:  
    Serial.print("OK,\t"); 
    //Splash Screen Water Temperature Sensor
    oled.clear();
    oled.setCursor(20,0);
    oled.println("DHT11 Sensor");
    oled.setCursor(0,10);
    oled.println("================");
    oled.setCursor(50,30);
    oled.println("OK");
    oled.setCursor(43,40);
    oled.println(DHT_LIB_VERSION);
  
    delay(1000);    
    break;
    case DHTLIB_ERROR_CHECKSUM: 
    Serial.print("Checksum error,\t"); 
    for(;;);
    break;
    case DHTLIB_ERROR_TIMEOUT: 
    Serial.print("Time out error,\t"); 
    for(;;);
    break;
    case DHTLIB_ERROR_CONNECT:
        Serial.print("Connect error,\t");
        for(;;);
        break;
    case DHTLIB_ERROR_ACK_L:
        Serial.print("Ack Low error,\t");
        for(;;);
        break;
    case DHTLIB_ERROR_ACK_H:
        Serial.print("Ack High error,\t");
        for(;;);
        break;
    default: 
    Serial.print("Unknown error,\t"); 
    for(;;);
    break;
  }
  
  oled.clear();
}
#pragma endregion

void loop() {

  ticks = millis();

  //20ms timer Ph Samples
  if((ticks - last_tick_20ms) > 20)
  {
    #pragma region Ph Sensor read
    
    PhArray[PhArrayIndex++]=analogRead(PhSensorPin);
    if (PhArrayIndex==PhArrayLenth) {
      PhArrayIndex=0;
    }   
    PhValue_mV=((30*(double)VOLTAGE*1000)-(75*avergearray(PhArray, PhArrayLenth)*VOLTAGE*1000/1024))/75-PH_OFFSET;   //convert the analog value to Ph_mV according the circuit

    #pragma endregion

    last_tick_20ms=ticks;
  }

  //1000ms timer
  if ((ticks - last_tick_1000ms) > 1000)
    { 

      #pragma region 1 second loop

      timmer_1s++;

      if(timmer_1s >= 1)
      {
        PrintValuestoOLED(); //Print values to OLED Display
        timmer_1s=0;
      }

      #pragma endregion

      #pragma region 5 seconds loop

      timmer_5s++;

      if(timmer_5s>5)
        {
        #pragma region Exterior Temp and Humidity
          Ext_Humidity = DHT.humidity;
          Serial.print("Current humidity = ");
          Serial.print(Ext_Humidity);
          Serial.print(" %");
          Serial.print("temperature = ");
          Ext_Temperature = DHT.temperature;
          Serial.print(Ext_Temperature); 
          Serial.println(" C");
        #pragma endregion 
        
        #pragma region Water Temperature Read
        //WaterTempSensor.requestTemperatures(); 
        //WaterTemp=WaterTempSensor.getTempCByIndex(0);      
        //Serial.print(" Water Temperature= ");
        //Serial.print(WaterTemp);
        #pragma endregion        
        
        #pragma region Water TDS Read
        SensorTDS.setTemperature(WaterTemp);  // set the temperature and execute temperature compensation
        SensorTDS.update();  //sample and calculate 
        tdsValue = SensorTDS.getTdsValue();  // then get the value
        Serial.print(tdsValue,0);
        Serial.println("ppm");
        #pragma endregion  
          
        #pragma region PH Sensor read
        //Line equation y=−0.016903313049357674x+7  or  y=−59.160000000000004x+414.12
        PhValue=(0.01690*PhValue_mV)+7;
        Serial.print("PH = ");
        Serial.println(PhValue,0);
        #pragma endregion        

        timmer_5s=0;
        }

        #pragma endregion

      #pragma region 1 minute loop

      timmer_1m++;

      if(timmer_1m>60)
      {

        timmer_1m=0;
      }

      #pragma endregion

      #pragma region 10 minute loop

      timmer_10m++;

      if(timmer_10m>600)
      {

        timmer_10m=0;
      }

      #pragma endregion

      #pragma region 30 minute loop

      timmer_30m++;

      if(timmer_30m>1800)
      {

        timmer_30m=0;
      }

      #pragma endregion

      #pragma region 1 hour loop

      timmer_1h++;

      if(timmer_1h>3600)
      {

        timmer_1h=0;
      }

      #pragma endregion
      
  
      last_tick_1000ms = ticks;
      
    } 

}

#pragma region Aux functions

#pragma region InitSensorTDS
void InitSensorTDS()
{
  SensorTDS.setPin(TdsSensorPin);
  SensorTDS.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  SensorTDS.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  SensorTDS.begin();  //initialization
}
#pragma endregion

#pragma region Ph Sensor Samples Average

double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    printf("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }
    }
    avg = (double)amount/(number-2);
  }

  return avg;
}

#pragma endregion

#pragma region OLED display print values

void PrintValuestoOLED()
{
  //clear display
  oled.clear();

  // display exterior temperature
  oled.print("Ext:");
  oled.print(Ext_Temperature,1);
  oled.print("'C");

  

  // display exterior humidity 
  oled.print("/R.H:");
  oled.print(Ext_Humidity,1);
  oled.println("%");

  // display water temperature
  oled.print("Water: ");
  oled.print(WaterTemp,1);
  oled.println("'C"); 
  

  // display pH Value
  oled.print("pH:  ");
  oled.println(PhValue,1);

  // display TDS Value
  oled.print("Tds: ");
  oled.print(tdsValue,1);
  oled.println(" ppm");
  
}

#pragma endregion

#pragma endregion
