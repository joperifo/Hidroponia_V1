#include <Fuzzy.h>


//Pins Define
   
#define WaterPump D0
#define Ph_Up_Valve D1
#define Nutrients_Valve D2
#define Water_Valve D3


//Global define


Fuzzy *fuzzy = new Fuzzy();


//Global Vars
bool FirstCycle = 1;
bool FuzzyOK_pH = false;

float input = 14;


uint32_t ticks, last_tick_20ms, last_tick_1000ms,timmer_1s,timmer_2s, timmer_5s, timmer_1m, timmer_10m, timmer_30m, timmer_1h ;



#pragma region Init Setup
void setup() {
  //Init serial port
  Serial.begin(115200);
  // Set a random seed, just for test purposes
  randomSeed(analogRead(0)); 

   //Init fuzzy logi controller for pH
   FuzzyOK_pH = InitFuzzyPh();
  
}
#pragma endregion

void loop() {
  // just for testing proposes
 
  ticks = millis();

  //20ms timer Ph Samples
  if((ticks - last_tick_20ms) > 20)
  {
    
    last_tick_20ms=ticks;
  }

  //1000ms timer
  if ((ticks - last_tick_1000ms) > 1000)
    { 

      #pragma region 1 second loop

      if(input <= 5.0)
        input = 14.0;
        
      input = input -0.3;
     

      Serial.println("\n\nInput:");
      Serial.print("\t\tPH:");
      Serial.println(input);
      // Set the random value as an input
      fuzzy->setInput(1, input);
      // Running the Fuzzification
      fuzzy->fuzzify();
      // Running the Defuzzification
      float output = fuzzy->defuzzify(1);
      // Printing something
      Serial.println("Result: ");
      Serial.print("\t\tPumpPulse: ");
      Serial.println(output);
      
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


#pragma endregion

#pragma region FuzzyLogic

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
  fuzzy->addFuzzyInput(ph);

  // Instantiating a FuzzyOutput objects
  FuzzyOutput *phPumpPulse = new FuzzyOutput(1);
  // Instantiating a FuzzySet object
  FuzzySet *small = new FuzzySet(50,200, 200, 300);
  // Including the FuzzySet into FuzzyOutput
  phPumpPulse->addFuzzySet(small);
  // Instantiating a FuzzySet object
  FuzzySet *medium = new FuzzySet(200, 400, 400, 500);
  // Including the FuzzySet into FuzzyOutput
  phPumpPulse->addFuzzySet(medium);
  // Instantiating a FuzzySet object
  FuzzySet *big = new FuzzySet(400, 500, 700, 800);
  // Including the FuzzySet into FuzzyOutput
  phPumpPulse->addFuzzySet(big);
  // Including the FuzzyOutput into Fuzzy
  fuzzy->addFuzzyOutput(phPumpPulse);
  
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
  fuzzy->addFuzzyRule(fuzzyRule01);

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
  fuzzy->addFuzzyRule(fuzzyRule02);

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
  fuzzy->addFuzzyRule(fuzzyRule03);  

  return true;
}

#pragma endregion

#pragma endregion
