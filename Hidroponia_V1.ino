#include <EEPROM.h>
#include <GravityTDS.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

#define TdsSensorPin A1

GravityTDS SensorTDS;

float temperature = 25,tdsValue = 0;

unsigned long ticks;
unsigned long last_tick_2000, last_tick_1000 ;

void setup() {

  //TDS Sensor
  InitSensorTDS();
  //Init Serial
  Serial.begin(115200);  

}

void loop() {

  ticks = millis();

  //2000ms timer
  if ((ticks - last_tick_2000) > 2000)
    {  
      //temperature = readTemperature();  //add your temperature sensor and read it
      SensorTDS.setTemperature(temperature);  // set the temperature and execute temperature compensation
      SensorTDS.update();  //sample and calculate 
      tdsValue = SensorTDS.getTdsValue();  // then get the value
      Serial.print(tdsValue,0);
      Serial.println("ppm");      
  
      last_tick_2000 = ticks;
    } 

}

void InitSensorTDS()
{
  SensorTDS.setPin(TdsSensorPin);
  SensorTDS.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  SensorTDS.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  SensorTDS.begin();  //initialization
}
