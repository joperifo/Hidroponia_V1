#include <Arduino.h>
#include <Fuzzy.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#endif

#define ON 1
#define OFF 0

#pragma region MQTT vars

// WiFi
const char* ssid = "JFMR@HOME";                 // Your personal network SSID
const char* wifi_password = "Misty@2019"; // Your personal network password

// MQTT
bool MqttEnable = false;
int WIFI_retries = 0;

const char* mqtt_server = "192.168.0.123";  // IP of the MQTT broker
const char* Exthumidity_topic_in = "Hidroponics/1/Sensors/Humidity";
const char* Exttemperature_topic_in = "Hidroponics/1/Sensors/Temperature";
const char* Watertemperature_topic_in = "Hidroponics/1/Sensors/WaterTemperature";
const char* pH_topic_in = "Hidroponics/1/Sensors/pH";
const char* TDS_topic_in = "Hidroponics/1/Sensors/TDS";
const char* nutrients_topic_out = "Hidroponics/1/Actuators/nutrients_pump";
const char* feed_topic_out = "Hidroponics/1/Actuators/feed_pump";
const char* pH_topic_out = "Hidroponics/1/Actuators/pH_pump";

const char* mqtt_username = "admin"; // MQTT username
const char* mqtt_password = "admin"; // MQTT password
const char* clientID = "NodeMCU-Hidro-1"; // MQTT client ID

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient); 
#pragma endregion

//Pins Define
//OLED Display I2C D1 and D2
#define BootModePin D0
#define Ph_Up_PumpPin D3
#define Nutrients_PumpPin D4
#define RxPin D6
#define TxPin D5

//LCD Define
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Software serial
SoftwareSerial s(RxPin,TxPin); //Rx D6 Tx D5

DeserializationError error;

//WiFi configuration vars

unsigned int  timeout   = 120; // seconds to run for
unsigned int  startTime = millis();
bool portalRunning      = false;
bool startAP            = false; // start AP and webserver if true, else start only webserver
int BootModeCounter500ms = 0;

WiFiManager wm;

//parameters define
int Tds_Value_ref=800;

//Range for Set 1 - slighty_low
int Fuzzy_tds_1_1 = Tds_Value_ref - (Tds_Value_ref/3);
int Fuzzy_tds_1_2 = Fuzzy_tds_1_1 + ((Tds_Value_ref/3)/3);
int Fuzzy_tds_1_3 = Fuzzy_tds_1_1 + 2*((Tds_Value_ref/3)/3);
int Fuzzy_tds_1_4 = Tds_Value_ref;
//Range for Set 1 - low
int Fuzzy_tds_2_1 = Tds_Value_ref - (2*(Tds_Value_ref/3));
int Fuzzy_tds_2_2 = Fuzzy_tds_2_1 + ((Tds_Value_ref/3)/3);
int Fuzzy_tds_2_3 = Fuzzy_tds_2_1 + 2*((Tds_Value_ref/3)/3);
int Fuzzy_tds_2_4 = Fuzzy_tds_1_1;
//Range for Set 1 - too_low
int Fuzzy_tds_3_1 = 0;
int Fuzzy_tds_3_2 = Fuzzy_tds_3_1 + ((Tds_Value_ref/3)/3);
int Fuzzy_tds_3_3 = Fuzzy_tds_3_1 + 2*((Tds_Value_ref/3)/3);
int Fuzzy_tds_3_4 = Fuzzy_tds_2_1;


//Global define
Fuzzy *fuzzy_pH = new Fuzzy();
Fuzzy *fuzzy_Nut = new Fuzzy();

//Global Vars
bool FirstCycle = 1;
bool FuzzyOK_pH = false;
bool FuzzyOK_Nutrients = false;

float Ph_Value = 0.0;
float Tds_Value = 0.0;
float ExtTemp = 0.0;
float WaterTemp = 0.0;
float ExtHum = 0.0;


float input = 14;

int DisplayRunning = 0;


uint32_t ticks, last_tick_20ms, last_tick_500ms, last_tick_1000ms,timmer_1s,timmer_2s, timmer_5s, timmer_1m, timmer_10m, timmer_30m, timmer_1h ;



#pragma region Init Setup
void setup() {

  FirstCycle=1;

  #pragma region Pins Init
  pinMode(RxPin, INPUT);
  pinMode(TxPin, OUTPUT);
  pinMode(Nutrients_PumpPin, OUTPUT);
  pinMode(Ph_Up_PumpPin, OUTPUT);
  pinMode(BootModePin, INPUT_PULLUP);

  #pragma endregion  

  #pragma region Init display
  //Init display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
  Serial.println("SSD1306 allocation failed");
  for(;;); // Don't proceed, loop forever
  }

  delay(2000);
  #pragma endregion

  #pragma region Init serial port
  //Init serial port
  Serial.begin(115200);
  while (!Serial) continue;

  display.clearDisplay();
  display.setCursor(20,5);
  display.println("Serial port 1");
  display.setCursor(0,15);
  display.println("=====================");
  display.setCursor(50,30);
  display.println("OK");
  display.setCursor(0,45);
  display.println("=====================");
  display.display();

  Serial.setDebugOutput(true);

  delay(1500);
#pragma endregion

  #pragma region Init Software Serial port
  //Init software serial port
  s.begin(4800);

  display.clearDisplay();
  display.setCursor(20,5);
  display.println("Serial port 2");
  display.setCursor(0,15);
  display.println("=====================");
  display.setCursor(50,30);
  display.println("OK");
  display.setCursor(0,45);
  display.println("=====================");
  display.display();

  delay(1500);
  #pragma endregion 

  #pragma region WiFi Portal Start
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  
  wm.resetSettings();
    
  wm.setHostname("Hidroponia");
  wm.autoConnect();

  #pragma endregion

  #pragma region Init Mqtt 
  if(MqttEnable)
  {
    bool mqtt_connected = false;

    WIFI_retries = 0;

    mqtt_connected = connect_MQTT();

    if(mqtt_connected)
    {
      display.clearDisplay();
      display.setCursor(20,5);
      display.println("MQTT");
      display.setCursor(0,15);
      display.println("Connected to:");
      display.setCursor(50,30);
      display.println(mqtt_server);
      display.setCursor(0,45);
      display.println("=====================");
      display.display();
    }
    else
    {
      display.clearDisplay();
      display.setCursor(20,5);
      display.println("MQTT");
      display.setCursor(0,15);
      display.println("=====================");
      display.setCursor(50,30);
      display.println("NOK");
      display.setCursor(0,45);
      display.println("=====================");
      display.display();

      for(;;);
    }
  }
    
  #pragma endregion

  #pragma region Print splash screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20,5);
  display.println("Hidroponia V1.0");
  display.setCursor(0,15);
  display.println("=====================");
  display.setCursor(25,30);
  display.println("Joao Fontes");
  display.setCursor(0,45);
  display.println("=====================");
  display.display();
  delay(1500);  
  // Set a random seed, just for test purposes
  randomSeed(analogRead(0)); 

  #pragma endregion

  #pragma region Init Fuzzy Logic pH pump
  //Init fuzzy logi controller for pH
  FuzzyOK_pH = InitFuzzyPh();  

  display.clearDisplay();
  display.setCursor(20,5);
  display.println("Fuzzy Logic");
  display.setCursor(0,15);
  display.println("=====================");
  display.setCursor(50,30);
  display.println("OK");
  display.setCursor(0,45);
  display.println("=====================");
  display.display();  
}
  #pragma endregion

#pragma endregion

void loop() {
  // just for testing proposes
  ticks = millis();

      #pragma region First cycle
  if(FirstCycle==ON)
  {
    display.clearDisplay();
    display.setCursor(0,5);
    display.println("RUNNING");
    display.setCursor(0,15);
    display.println("                      ");
    display.setCursor(0,30);
    display.println("                      ");
    display.setCursor(0,45);
    display.println("                      ");
    display.setCursor(0,60);
    display.println("                      ");
    display.display();

    FirstCycle=OFF;
  } 
  
  #pragma endregion

  #pragma region Getting JSON info from arduino
  StaticJsonDocument <1700> jsonDoc;
  error = deserializeJson(jsonDoc, s);
  if (error)
  {
    Serial.println("INVALID JSON FROM ARDUINO!!!");
    //return;
  }
  Ph_Value=jsonDoc["Ph_Value"];
  Tds_Value=jsonDoc["Tds_Value"];
  ExtTemp=jsonDoc["ExtTemp"];
  ExtHum=jsonDoc["ExtHum"];
  WaterTemp=jsonDoc["WaterTemp"];  

  #pragma endregion

  #pragma region 20 ms loop
  //20ms timer Ph Samples
  if((ticks - last_tick_20ms) > 20)
  {

    
    
    last_tick_20ms=ticks;
  }
  #pragma endregion

  #pragma region 500 ms loop
  //500ms timer 
  if((ticks - last_tick_500ms) > 500)
  {    
    
      
    last_tick_500ms=ticks;
  }
  #pragma endregion

  #pragma region 1 second loop

  //1000ms timer
  if ((ticks - last_tick_1000ms) > 1000)
    {       
      #pragma region Display Update
      if(DisplayRunning == 1)
      {
        display.clearDisplay();
        display.setCursor(0,5);
        display.println("RUNNING");
        display.setCursor(120,5);
        display.cp437(true);
        display.write(0);          
        display.display();

        DisplayRunning=0;
      }   
      else
      {
        display.clearDisplay();
        display.setCursor(0,5);
        display.println("RUNNING");
        display.setCursor(120,5);
        display.cp437(true);
        display.write(16);
        display.display();

        DisplayRunning=1;
      }      
      #pragma endregion   
      
      //only for testing
      if(input <= 5.0)
        input = 14.0;
        
      input = input -0.3;
     
     #pragma region Print JSON values to Serial
      if(!error)
      {
          Serial.println("\n----------------------------------------");
          Serial.println("Values from sensors board");
          Serial.print("JSON pH Value=");
          Serial.println(Ph_Value);
          Serial.print("JSON Tds Value=");
          Serial.println(Tds_Value);
          Serial.print("JSON External Temperature=");
          Serial.println(ExtTemp);
          Serial.print("JSON External Humidity=");
          Serial.println(ExtHum);
          Serial.print("JSON Water Temperature=");
          Serial.println(WaterTemp);
          Serial.println("----------------------------------------");

      }
      #pragma endregion
      
      Serial.println("Result Fuzzy logic pH output");
      Serial.println("\nInput:");
      Serial.print("\tPH:");
      Serial.println(Ph_Value);
      // Set the random value as an input for pH
      fuzzy_pH->setInput(1, Ph_Value);
      // Running the Fuzzification for pH
      fuzzy_pH->fuzzify();
      // Running the Defuzzification for pH
      float output = fuzzy_pH->defuzzify(1);
      // Printing something
      Serial.println("Result: ");
      Serial.print("\tPumpPulse: ");
      Serial.println(output);
      Serial.println("----------------------------------------");

      //Nutrients control from Tds value
      Serial.println("Result Fuzzy logic Nutrients output");
      Serial.print("\tTds:");
      Serial.println(Tds_Value);
      // Set the random value as an input for Tds
      fuzzy_Nut->setInput(1, Tds_Value);
      // Running the Fuzzification for Tds
      fuzzy_Nut->fuzzify();
      // Running the Defuzzification for Tds
      float output_nut = fuzzy_Nut->defuzzify(1);
      // Printing something
      Serial.println("Result: ");
      Serial.print("\tNutrients PumpPulse: ");
      Serial.println(output_nut);
      Serial.println("----------------------------------------");

      
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

        #ifdef ESP8266
        MDNS.update();
        #endif
        doWiFiManager();        
        
        timmer_2s=0;
        
      }

      #pragma endregion

  #pragma region 5 seconds loop

      timmer_5s++;

      if(timmer_5s>5 or FirstCycle)
        {                       

                 
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
    
        #pragma region Send MQTT values

        if(MqttEnable)
        {

          #pragma region Construct MQTT strings
          // MQTT construct strings to send
          /*String HumExtSensor="ExtHum: "+String((float)ExtHum)+" % ";
          String TempExtSensor="ExtTemp: "+String((float)ExtTemp)+" C ";
          String TempWaterSensor="WaterTemp: "+String((float)WaterTemp)+" C ";
          String pHWaterSensor="WaterPH: "+String((float)Ph_Value);
          String TdsWaterSensor="WaterTDS: "+String((float)Tds_Value)+" ppm ";*/
          #pragma endregion

          #pragma region Send External humidity to MQTT
          if (client.publish(Exthumidity_topic_in, String(ExtHum).c_str())) {
            Serial.println("External Humidity sent!");
          }
          else {
            Serial.println("External Humidity failed to send. Reconnecting to MQTT Broker and trying again");
            client.connect(clientID, mqtt_username, mqtt_password);
            delay(10); 
            client.publish(Exthumidity_topic_in, String(ExtHum).c_str());
          }
          #pragma endregion

          #pragma region Send External temperature to MQTT
          if (client.publish(Exttemperature_topic_in, String(ExtTemp).c_str())) {
            Serial.println("External Temperature sent!");
          }
          else {
            Serial.println("External Temperature failed to send. Reconnecting to MQTT Broker and trying again");
            client.connect(clientID, mqtt_username, mqtt_password);
            delay(10); 
            client.publish(Exttemperature_topic_in, String(ExtTemp).c_str());
          }
          #pragma endregion

          #pragma region Send Water temperature to MQTT
          if (client.publish(Watertemperature_topic_in, String(WaterTemp).c_str())) {
            Serial.println("Water Temperature sent!");
          }
          else {
            Serial.println("Water Temperature failed to send. Reconnecting to MQTT Broker and trying again");
            client.connect(clientID, mqtt_username, mqtt_password);
            delay(10); 
            client.publish(Watertemperature_topic_in, String(WaterTemp).c_str());
          }
          #pragma endregion

          #pragma region Send Water pH to MQTT
          if (client.publish(pH_topic_in, String(Ph_Value).c_str())) {
            Serial.println("Water pH sent!");
          }
          else {
            Serial.println("Water pH failed to send. Reconnecting to MQTT Broker and trying again");
            client.connect(clientID, mqtt_username, mqtt_password);
            delay(10); 
            client.publish(pH_topic_in, String(Ph_Value).c_str());
          }
          #pragma endregion

          #pragma region Send Water Tds to MQTT
          if (client.publish(TDS_topic_in, String(Tds_Value).c_str())) {
            Serial.println("Water Tds sent!");
          }
          else {
            Serial.println("Water Tds failed to send. Reconnecting to MQTT Broker and trying again");
            client.connect(clientID, mqtt_username, mqtt_password);
            delay(10); 
            client.publish(TDS_topic_in, String(Tds_Value).c_str());
          }
          #pragma endregion
        
        }
        #pragma endregion

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
  
      last_tick_1000ms = ticks;
      
    } 
    #pragma endregion


}

#pragma region Aux functions

 #pragma region MQTT Aux Functions

  bool connect_MQTT(){
    Serial.print("Connecting to ");
    Serial.println(ssid);

    // Connect to the WiFi
    WiFi.begin(ssid, wifi_password);

    // Wait until the connection has been confirmed before continuing
    while (WiFi.status() != WL_CONNECTED && WIFI_retries < 10)  {
      delay(500);
      Serial.print(".");
      WIFI_retries++;
    }
    if(WIFI_retries >= 10)
    {
      return false;
    }

    // Debugging - Output the IP Address of the ESP8266
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Connect to MQTT Broker
    // client.connect returns a boolean value to let us know if the connection was successful.
    // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
    if (client.connect(clientID, mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT Broker!");
    }
    else {
      Serial.println("Connection to MQTT Broker failed...");
    }

    return true;
}
 
 #pragma endregion

#pragma region WiFi portal Aux functions

void doWiFiManager(){
  // is auto timeout portal running
  if(portalRunning){
    wm.process();
    if((millis()-startTime) > (timeout*1000)){
      Serial.println("portaltimeout");
      portalRunning = false;
      if(startAP){
        wm.stopConfigPortal();
      }  
      else{
        wm.stopWebPortal();
      } 
   }
  }

  // is configuration portal requested?
  if(digitalRead(BootModePin) == LOW && (!portalRunning)) {
    if(startAP){
      Serial.println("Button Pressed, Starting Config Portal");
      wm.setConfigPortalBlocking(false);
      wm.startConfigPortal();
    }  
    else{
      Serial.println("Button Pressed, Starting Web Portal");
      wm.startWebPortal();
    }  
    portalRunning = true;
    startTime = millis();
  }
}

#pragma endregion

#pragma endregion

#pragma region FuzzyLogic

#pragma region pH pump
  bool InitFuzzyPh()
  {
    // Instantiating a FuzzyInput object
    FuzzyInput *ph = new FuzzyInput(1);
    // Instantiating a FuzzySet object
    FuzzySet *acid = new FuzzySet(6.0, 6.5, 7.0, 7.5);
    // Including the FuzzySet into FuzzyInput
    ph->addFuzzySet(acid);
    // Instantiating a FuzzySet object
    FuzzySet *neutral = new FuzzySet(7.0, 7.5, 8.0, 8.5);
    // Including the FuzzySet into FuzzyInput
    ph->addFuzzySet(neutral);
    // Instantiating a FuzzySet object
    FuzzySet *base = new FuzzySet(8.0, 8.5, 10.0, 14.0);
    // Including the FuzzySet into FuzzyInput
    ph->addFuzzySet(base);
    // Including the FuzzyInput into Fuzzy
    fuzzy_pH->addFuzzyInput(ph);

    // Instantiating a FuzzyOutput objects
    FuzzyOutput *phPumpPulse = new FuzzyOutput(1);
    // Instantiating a FuzzySet object
    FuzzySet *small = new FuzzySet(500,2000, 2000, 3000);
    // Including the FuzzySet into FuzzyOutput
    phPumpPulse->addFuzzySet(small);
    // Instantiating a FuzzySet object
    FuzzySet *medium = new FuzzySet(2000, 4000, 4000, 5000);
    // Including the FuzzySet into FuzzyOutput
    phPumpPulse->addFuzzySet(medium);
    // Instantiating a FuzzySet object
    FuzzySet *big = new FuzzySet(4000, 5000, 7000, 8000);
    // Including the FuzzySet into FuzzyOutput
    phPumpPulse->addFuzzySet(big);
    // Including the FuzzyOutput into Fuzzy
    fuzzy_pH->addFuzzyOutput(phPumpPulse);
    
    // Building FuzzyRule "IF ph = acid THEN phPumpPulse = small"
    // Instantiating a FuzzyRuleAntecedent objects
    FuzzyRuleAntecedent *ifPhAcid = new FuzzyRuleAntecedent();
    // Creating a FuzzyRuleAntecedent with just a single FuzzySet
    ifPhAcid->joinSingle(acid);
    // Instantiating a FuzzyRuleConsequent objects
    FuzzyRuleConsequent *thenphPumpPulseSmall = new FuzzyRuleConsequent();
    // Including a FuzzySet to this FuzzyRuleConsequent
    thenphPumpPulseSmall->addOutput(small);
    // Instantiating a FuzzyRule objects
    FuzzyRule *fuzzyRule01 = new FuzzyRule(1, ifPhAcid, thenphPumpPulseSmall);
    // Including the FuzzyRule into Fuzzy
    fuzzy_pH->addFuzzyRule(fuzzyRule01);

    // Building FuzzyRule "IF ph = neutral THEN phPumpPulse = medium"
    // Instantiating a FuzzyRuleAntecedent objects
    FuzzyRuleAntecedent *ifPhNeutral = new FuzzyRuleAntecedent();
    // Creating a FuzzyRuleAntecedent with just a single FuzzySet
    ifPhNeutral->joinSingle(neutral);
    // Instantiating a FuzzyRuleConsequent objects
    FuzzyRuleConsequent *thenphPumpPulseMedium = new FuzzyRuleConsequent();
    // Including a FuzzySet to this FuzzyRuleConsequent
    thenphPumpPulseMedium->addOutput(medium);
    // Instantiating a FuzzyRule objects
    FuzzyRule *fuzzyRule02 = new FuzzyRule(2, ifPhNeutral, thenphPumpPulseMedium);
    // Including the FuzzyRule into Fuzzy
    fuzzy_pH->addFuzzyRule(fuzzyRule02);

    // Building FuzzyRule "IF ph = base THEN phPumpPulse = big"
    // Instantiating a FuzzyRuleAntecedent objects
    FuzzyRuleAntecedent *ifPhBase = new FuzzyRuleAntecedent();
    // Creating a FuzzyRuleAntecedent with just a single FuzzySet
    ifPhBase->joinSingle(base);
    // Instantiating a FuzzyRuleConsequent objects
    FuzzyRuleConsequent *thenphPumpPulseBig = new FuzzyRuleConsequent();
    // Including a FuzzySet to this FuzzyRuleConsequent
    thenphPumpPulseBig->addOutput(big);
    // Instantiating a FuzzyRule objects
    FuzzyRule *fuzzyRule03 = new FuzzyRule(3, ifPhBase, thenphPumpPulseBig);
    // Including the FuzzyRule into Fuzzy
    fuzzy_pH->addFuzzyRule(fuzzyRule03);  

    return true;
}
#pragma endregion

#pragma region Nutrients pump
bool InitFuzzyNutrients()
{
  // Instantiating a FuzzyInput object
  FuzzyInput *tds_value = new FuzzyInput(1);
  // Instantiating a FuzzySet object
  FuzzySet *slightly_low = new FuzzySet(Fuzzy_tds_1_1, Fuzzy_tds_1_2, Fuzzy_tds_1_3, Fuzzy_tds_1_4);
  // Including the FuzzySet into FuzzyInput
  tds_value->addFuzzySet(slightly_low);
  // Instantiating a FuzzySet object
  FuzzySet *low = new FuzzySet(Fuzzy_tds_2_1, Fuzzy_tds_2_2, Fuzzy_tds_2_3, Fuzzy_tds_2_4);
  // Including the FuzzySet into FuzzyInput
  tds_value->addFuzzySet(low);
  // Instantiating a FuzzySet object
  FuzzySet *too_low = new FuzzySet(Fuzzy_tds_3_1, Fuzzy_tds_3_2, Fuzzy_tds_3_3, Fuzzy_tds_3_4);
  // Including the FuzzySet into FuzzyInput
  tds_value->addFuzzySet(too_low);
  // Including the FuzzyInput into Fuzzy
  fuzzy_Nut->addFuzzyInput(tds_value);

  // Instantiating a FuzzyOutput objects
  FuzzyOutput *NutPumpPulse = new FuzzyOutput(1);
  // Instantiating a FuzzySet object
  FuzzySet *small = new FuzzySet(500,2000, 2000, 3000);
  // Including the FuzzySet into FuzzyOutput
  NutPumpPulse->addFuzzySet(small);
  // Instantiating a FuzzySet object
  FuzzySet *medium = new FuzzySet(2000, 4000, 4000, 5000);
  // Including the FuzzySet into FuzzyOutput
  NutPumpPulse->addFuzzySet(medium);
  // Instantiating a FuzzySet object
  FuzzySet *big = new FuzzySet(4000, 5000, 7000, 8000);
  // Including the FuzzySet into FuzzyOutput
  NutPumpPulse->addFuzzySet(big);
  // Including the FuzzyOutput into Fuzzy
  fuzzy_Nut->addFuzzyOutput(NutPumpPulse);

  // Building FuzzyRule "IF Tds value = slighlty_low THEN pump pulse = small"
  // Instantiating a FuzzyRuleAntecedent objects
  FuzzyRuleAntecedent *ifTdsSlightlyLow = new FuzzyRuleAntecedent();
  // Creating a FuzzyRuleAntecedent with just a single FuzzySet
  ifTdsSlightlyLow->joinSingle(slightly_low);
  // Instantiating a FuzzyRuleConsequent objects
  FuzzyRuleConsequent *thenNutPumpPulseSmall = new FuzzyRuleConsequent();
  // Including a FuzzySet to this FuzzyRuleConsequent
  thenNutPumpPulseSmall->addOutput(small);
  // Instantiating a FuzzyRule objects
  FuzzyRule *fuzzyRule01 = new FuzzyRule(1, ifTdsSlightlyLow, thenNutPumpPulseSmall);
  // Including the FuzzyRule into Fuzzy
  fuzzy_Nut->addFuzzyRule(fuzzyRule01);

  // Building FuzzyRule "IF Tds value = low THEN pump pulse = medium"
  // Instantiating a FuzzyRuleAntecedent objects
  FuzzyRuleAntecedent *ifTdsLow = new FuzzyRuleAntecedent();
  // Creating a FuzzyRuleAntecedent with just a single FuzzySet
  ifTdsLow->joinSingle(low);
  // Instantiating a FuzzyRuleConsequent objects
  FuzzyRuleConsequent *thenNutPumpPulseMedium = new FuzzyRuleConsequent();
  // Including a FuzzySet to this FuzzyRuleConsequent
  thenNutPumpPulseMedium->addOutput(medium);
  // Instantiating a FuzzyRule objects
  FuzzyRule *fuzzyRule02 = new FuzzyRule(2, ifTdsLow, thenNutPumpPulseMedium);
  // Including the FuzzyRule into Fuzzy
  fuzzy_Nut->addFuzzyRule(fuzzyRule02);

  // Building FuzzyRule "IF Tds value = too_low THEN pump pulse = Big"
  // Instantiating a FuzzyRuleAntecedent objects
  FuzzyRuleAntecedent *ifTdsTooLow = new FuzzyRuleAntecedent();
  // Creating a FuzzyRuleAntecedent with just a single FuzzySet
  ifTdsTooLow->joinSingle(too_low);
  // Instantiating a FuzzyRuleConsequent objects
  FuzzyRuleConsequent *thenNutPumpPulseBig = new FuzzyRuleConsequent();
  // Including a FuzzySet to this FuzzyRuleConsequent
  thenNutPumpPulseBig->addOutput(big);
  // Instantiating a FuzzyRule objects
  FuzzyRule *fuzzyRule03 = new FuzzyRule(3, ifTdsTooLow, thenNutPumpPulseBig);
  // Including the FuzzyRule into Fuzzy
  fuzzy_Nut->addFuzzyRule(fuzzyRule03);
}
#pragma endregion

#pragma endregion

#pragma endregion
