#include <eeprom.h>
#include <GravityTDS.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <DHT.h>

#define TdsSensorPin A1
#define ONE_WIRE_BUS 5
#define DHT11_PIN 4

GravityTDS SensorTDS;

float temperature = 25,tdsValue = 0;

uint32_t ticks, last_tick_2000, last_tick_1000 ;
float WaterTemp=0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature WaterTempSensor(&oneWire);

#pragma region Init Setup
void setup() {
  //Init Serial
  Serial.begin(115200);
  //TDS Sensor
  InitSensorTDS();
  //Water Temperature Sensor
  WaterTempSensor.begin();
  //DHT11 Test
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT_LIB_VERSION);
  Serial.println();
  
}
#pragma endregion

void loop() {

  ticks = millis();

  //2000ms timer
  if ((ticks - last_tick_2000) > 2000)
    { 
      #pragma region Exterior Temp and Humidity
            
      #pragma endregion 

      #pragma region Water Temperature Read
      sensors.requestTemperatures(); 
      WaterTemp=sensors.getTempCByIndex(0);      
      Serial.print(" Water Temperature= ");
      Serial.print(Celcius);
      #pragma endregion
      
      #pragma region Water TDS Read
      SensorTDS.setTemperature(WaterTemp);  // set the temperature and execute temperature compensation
      SensorTDS.update();  //sample and calculate 
      tdsValue = SensorTDS.getTdsValue();  // then get the value
      Serial.print(tdsValue,0);
      Serial.println("ppm");
      #pragma endregion  
  
      last_tick_2000 = ticks;
    } 

}

#pragma region InitSensorTDS
void InitSensorTDS()
{
  SensorTDS.setPin(TdsSensorPin);
  SensorTDS.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  SensorTDS.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  SensorTDS.begin();  //initialization
}
#pragma endregion
