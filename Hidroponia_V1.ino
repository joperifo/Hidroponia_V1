#include <EEPROM.h>
#include <dht.h>
#include <OneWire.h>
#include <GravityTDS.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//Pins Define
#define TdsSensorPin A0
#define PhSensorPin A1          
#define DHT11_PIN A2
#define ONE_WIRE_BUS 2
#define WaterPump 3
#define Ph_Up_Valve 4
#define Nutrients_Valve 5
#define Water_Valve 6

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

//Global define
#define VOLTAGE 5.00    //system voltage
#define PH_OFFSET 0        //zero drift voltage
#define PhArrayLenth  40    //PH sensor number of samples
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

GravityTDS SensorTDS;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature WaterTempSensor(&oneWire);
dht DHT;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Global Vars
int PhArray[PhArrayLenth];
int PhArrayIndex=0;
double PhValue_mV;
float PhValue;

float temperature = 25,tdsValue = 0;

uint32_t ticks, last_tick_20ms, last_tick_1000ms,timmer_1s, timmer_5s, timmer_1m, timmer_10m, timmer_30m, timmer_1h ;
float WaterTemp=0;



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
  //OLED display
  //Wire.begin();
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Init display at 0x3C I2C Addr
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
  Serial.println("OLED Display - SSD1306 allocation failed");
  for(;;); // Don't proceed, loop forever
  }  
  display.setTextColor(WHITE); //OLED display Text color
  display.setTextSize(1); //OLED display text size
  display.clearDisplay(); //Clear display

  display.setCursor(10,10); //POSIÇÃO EM QUE O CURSOR IRÁ FAZER A ESCRITA
  display.print("Hidroponia V1.0"); //ESCREVE O TEXTO NO DISPLAY
  display.display(); //EFETIVA A ESCRITA NO DISPLAY
  display.setCursor(10,40); //POSIÇÃO EM QUE O CURSOR IRÁ FAZER A ESCRITA
  display.print("OLED Display Started"); //ESCREVE O TEXTO NO DISPLAY
  display.display(); //EFETIVA A ESCRITA NO DISPLAY
  delay(2000);
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

        timmer_1s=0;
      }

      #pragma endregion

      #pragma region 5 seconds loop

      timmer_5s++;

      if(timmer_5s>5)
        {
        #pragma region Exterior Temp and Humidity
          DHT.read11(DHT11_PIN);
          Serial.print("Current humidity = ");
          Serial.print(DHT.humidity);
          Serial.print("%  ");
          Serial.print("temperature = ");
          Serial.print(DHT.temperature); 
          Serial.println("C  ");
        #pragma endregion 
        
        #pragma region Water Temperature Read
        WaterTempSensor.requestTemperatures(); 
        WaterTemp=WaterTempSensor.getTempCByIndex(0);      
        Serial.print(" Water Temperature= ");
        Serial.print(WaterTemp);
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
        Serial.println("PH = ");
        Serial.println(PhValue,0);
        #pragma endregion

        PrintValuestoOLED(); //Print values to OLED Display

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
  display.clearDisplay();

  // display exterior temperature
  display.setTextSize(1);
  display.setCursor(5,0);
  display.print("Exterior: ");
  display.setTextSize(2);
  display.setCursor(5,60);
  display.print(DHT.temperature);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");

  // display exterior humidity
  display.setTextSize(1);
  display.setCursor(5, 100);
  display.print(" / ");
  display.setTextSize(2);
  display.setCursor(5, 120);
  display.print(DHT.humidity);
  display.print(" %"); 

  // display water temperature
  display.setTextSize(1);
  display.setCursor(40,0);
  display.print("Water: ");
  display.setTextSize(2);
  display.setCursor(40,60);
  display.print(WaterTemp);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");

  // display pH Value
  display.setTextSize(1);
  display.setCursor(80, 0);
  display.print("pH: ");
  display.setTextSize(2);
  display.setCursor(80, 50);
  display.print(PhValue);

  // display TDS Value
  display.setTextSize(1);
  display.setCursor(80, 100);
  display.print("Tds: ");
  display.setTextSize(2);
  display.setCursor(80, 140);
  display.print(tdsValue);
  display.print(" ppm");
 
  
  display.display();
  
}

#pragma endregion

#pragma endregion
