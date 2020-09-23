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
#define VREF 5.0 // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point

#define I2C_ADDRESS 0x3C    //OLED Display
#define RST_PIN -1 // Reset pin # (or -1 if sharing Arduino reset pin)

SSD1306AsciiWire oled;
dht DHT;

GravityTDS SensorTDS;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature WaterTempSensor(&oneWire);

//Global Vars
bool FirstCycle = 1;
int PhArray[PhArrayLenth];
int PhArrayIndex=0;
double PhValue_mV;
float PhValue;
float Ext_Humidity, Ext_Temperature;

//Ph vars
unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;


int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0;

uint32_t ticks, last_tick_20ms,last_tick_40ms, last_tick_800ms, last_tick_1000ms,timmer_1s,timmer_2s, timmer_5s, timmer_1m, timmer_10m, timmer_30m, timmer_1h ;
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

  delay(1500);
  
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
    Serial.println("OK\t"); 
    Serial.println(); 
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
    Serial.println("Checksum error,\t"); 
    oled.clear();
    oled.setCursor(20,0);
    oled.println("DHT11 Sensor");
    oled.setCursor(0,10);
    oled.println("================");
    oled.setCursor(50,30);
    oled.println("NOK 1");
    oled.setCursor(43,40);
    oled.println(DHT_LIB_VERSION);
    for(;;);
    break;
    case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("Time out error,\t"); 
    oled.clear();
    oled.setCursor(20,0);
    oled.println("DHT11 Sensor");
    oled.setCursor(0,10);
    oled.println("================");
    oled.setCursor(50,30);
    oled.println("NOK 2");
    oled.setCursor(43,40);
    oled.println(DHT_LIB_VERSION);
    for(;;);
    break;
    case DHTLIB_ERROR_CONNECT:
        Serial.println("Connect error,\t");
        oled.clear();
        oled.setCursor(20,0);
        oled.println("DHT11 Sensor");
        oled.setCursor(0,10);
        oled.println("================");
        oled.setCursor(15,30);
        oled.println("Not Connected!");
        oled.setCursor(43,40);
        oled.println(DHT_LIB_VERSION);
        for(;;);
        break;
    case DHTLIB_ERROR_ACK_L:
        Serial.println("Ack Low error,\t");
        oled.clear();
        oled.setCursor(20,0);
        oled.println("DHT11 Sensor");
        oled.setCursor(0,10);
        oled.println("================");
        oled.setCursor(50,30);
        oled.println("NOK 3");
        oled.setCursor(43,40);
        oled.println(DHT_LIB_VERSION);
        for(;;);
        break;
    case DHTLIB_ERROR_ACK_H:
        Serial.println("Ack High error,\t");
        oled.clear();
        oled.setCursor(20,0);
        oled.println("DHT11 Sensor");
        oled.setCursor(0,10);
        oled.println("================");
        oled.setCursor(50,30);
        oled.println("NOK 4");
        oled.setCursor(43,40);
        oled.println(DHT_LIB_VERSION);
        for(;;);
        break;
    default: 
    Serial.println("Unknown error,\t"); 
    oled.clear();
    oled.setCursor(20,0);
    oled.println("DHT11 Sensor");
    oled.setCursor(0,10);
    oled.println("================");
    oled.setCursor(50,30);
        oled.println("NOK 5");
    oled.setCursor(43,40);
    oled.println(DHT_LIB_VERSION);
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
    //PhValue_mV=((30*(double)VOLTAGE*1000)-(75*avergearray(PhArray, PhArrayLenth)*VOLTAGE*1000/1024))/75-PH_OFFSET;   //convert the analog value to Ph_mV according the circuit
    PhValue=(float)avergearray(PhArray, PhArrayLenth)*5.0/1024/6; //convert the analog into millivolt
    #pragma endregion

    last_tick_20ms=ticks;
  }

  //40ms timer
  if((ticks - last_tick_40ms) > 40)
    {
      #pragma region TDS sample collect

      analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); //read the analog value and store into the buffer
      analogBufferIndex++;
      if(analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;

      #pragma endregion

      last_tick_40ms=ticks;
    }


  //800ms timer
  if((ticks - last_tick_800ms) > 800)
    {
      #pragma region Calculate tds value with temperature compensation

      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
      analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF/ 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(WaterTemp-25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient; //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V ");

      #pragma endregion

      last_tick_800ms=ticks;
    }
  //1000ms timer
  if ((ticks - last_tick_1000ms) > 1000)
    { 

      #pragma region 1 second loop

      timmer_1s++;

      if(timmer_1s >= 1)
      {
        timmer_1s=0;        
      }

      #pragma endregion

      #pragma region 2 second loop

      timmer_2s++;

      if(timmer_2s >= 2 or FirstCycle)
      {
                
        
        timmer_2s=0;
        
      }

      #pragma endregion

      #pragma region 5 seconds loop

      timmer_5s++;

      if(timmer_5s>5 or FirstCycle)
        {                   

        #pragma region Exterior Temp and Humidity
        DHT.read11(DHT11_PIN);
        Ext_Humidity = DHT.humidity;
        Serial.println("-----NEW DATA-----");
        Serial.print("Current humidity = ");
        Serial.print(Ext_Humidity);
        Serial.print(" %");
        Serial.print("    temperature = ");
        Ext_Temperature = DHT.temperature;
        Serial.print(Ext_Temperature); 
        Serial.println(" C");
        #pragma endregion

        #pragma region Water Temperature Read
        WaterTempSensor.requestTemperatures(); 
        WaterTemp=WaterTempSensor.getTempCByIndex(0);      
        Serial.print("Water Temperature= ");
        Serial.print(WaterTemp);
        Serial.println(" C");
        #pragma endregion        
        
        #pragma region Print Water TDS Read
        Serial.println("-----TDS-----");
        Serial.print(tdsValue,0);
        Serial.println("ppm");
        Serial.print("voltage:");
        Serial.print(averageVoltage,2);
        Serial.println("V ");
        #pragma endregion  
          
        #pragma region PH Sensor read
        Serial.println("-----PH-----");
        //Line equation y=−0.016903313049357674x+7  or  y=−59.160000000000004x+414.12
        //PhValue=(0.01690*PhValue_mV)+7;
        PhValue=3.5*PhValue;                      //convert the millivolt into pH value
        Serial.print("PH = ");
        Serial.println(PhValue,2);
        #pragma endregion
        
        PrintValuestoOLED(); //Print values to OLED Display
         
        timmer_5s=0;
        FirstCycle=false;

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
  pinMode(TdsSensorPin,INPUT);
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
  oled.print(PhValue,1);
  oled.println("  ");

  // display TDS Value
  oled.print("Tds: ");
  oled.print(tdsValue,1);
  oled.println(" ppm  ");
  
}

#pragma endregion

#pragma region TDS Aux functions

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
  for (i = 0; i < iFilterLen - j - 1; i++)
  {
  if (bTab[i] > bTab[i + 1])
  {
  bTemp = bTab[i];
  bTab[i] = bTab[i + 1];
  bTab[i + 1] = bTemp;
  }
  }
  }
  if ((iFilterLen & 1) > 0)
  bTemp = bTab[(iFilterLen - 1) / 2];
  else
  bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
#pragma endregion

#pragma endregion
