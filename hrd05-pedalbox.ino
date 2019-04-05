/*
      __  __                          ____             _                ____  _       _      _
     / / / /___ _____  ____  ___     / __ \____ ______(_)___  ____ _   / __ \(_)   __(_)____(_)___  ____
    / /_/ / __ `/ __ \/_  / / _ \   / /_/ / __ `/ ___/ / __ \/ __ `/  / / / / / | / / / ___/ / __ \/ __ \
   / __  / /_/ / / / / / /_/  __/  / _, _/ /_/ / /__/ / / / / /_/ /  / /_/ / /| |/ / (__  ) / /_/ / / / /
  /_/ /_/\__,_/_/ /_/ /___/\___/  /_/ |_|\__,_/\___/_/_/ /_/\__, /  /_____/_/ |___/_/____/_/\____/_/ /_/

  Pedalbox code for the HRD05
  Mark Oosting, 2019
  Joram de Poel, 2019
  Vincent van der Woude, 2019

*/

#include <CAN.h> // https://github.com/sandeepmistry/arduino-CAN

#define APS1_IN 0
#define APS2_IN 1
#define BPS1_IN 2
#define BPS2_IN 3

float MAX_DIFF = 0.075;

byte THROTTLE = 0;
byte BRAKE    = 0;
 
float POT_RATIO = 0.74;

int APS1_OFFSET = 0;
int APS2_OFFSET = 0;
int BPS1_OFFSET = 0;
int BPS2_OFFSET = 0;

// Limits of sensor output
int APS1_LOW = 784;
int APS2_LOW = 349;
int BPS1_LOW = 784;
int BPS2_LOW = 349;
int APS1_HIGH = 349;
int APS2_HIGH = 784;
int BPS1_HIGH = 349;
int BPS2_HIGH = 784;

unsigned long last = 0;     //millis when last loop was executed
int DATA_RATE = 20;          // Send CAN frame every 20 ms (and read sensors continuously)

/* ERROR STATUS:
  SAFE STATUS:        0
  STARTUP STATUS:     1
  APS1 OUT OF RANGE:  11
  APS2 OUT OF RANGE:  12
  BPS1 OUT OF RANGE:  21
  BPS2 OUT OF RANGE:  22
  APS IMPLAUSIBLE:    30
  BPS IMPLAUSIBLE:    40
  BSPD:               50
*/
byte STATUS = 1;

byte BUFFERSIZE = 50
//moving average for sensors
int LPF_APS1[BUFFERSIZE];           
int LPF_APS2[BUFFERSIZE];
int LPF_BPS1[BUFFERSIZE];
int LPF_BPS2[BUFFERSIZE];
//average value after moving average
long AV_APS1 = 0;          
long AV_APS2 = 0;
long AV_BPS1 = 0;
long AV_BPS2 = 0;

void setup() {
  //Get initial values to fill arrays:
  AV_APS1 = analogRead(APS1_IN);  
  AV_APS2 = analogRead(APS2_IN);
  AV_BPS1 = analogRead(BPS1_IN);
  AV_BPS2 = analogRead(BPS2_IN);
  // Run for loop to fill array:
  for(int i=0; i<BUFFERSIZE; i++){  
    LPF_APS1[i] = AV_APS1
    LPF_APS2[i] = AV_APS2
    LPF_BPS1[i] = AV_BPS1
    LPF_BPS2[i] = AV_BPS2
  }

  Serial.begin(115200);

  // Start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  if(STATUS==1) STATUS=0;
  last = millis();
}

void loop() {
  // Update moving average for pedal sensors:
  // Average to 0:
  AV_APS1 = 0;                        
  AV_APS2 = 0;
  AV_BPS1 = 0;
  AV_BPS2 = 0;

  for (int i = 0; i < (BUFFERSIZE-1); i++) {
    // Add value for average:
    AV_APS1 += LPF_APS1[i];             
    AV_APS2 += LPF_APS2[i];
    AV_BPS1 += LPF_BPS1[i];
    AV_BPS2 += LPF_BPS2[i];
    // Shift all values by one:
    LPF_APS1[i+1] = LPF_APS1[i];        
    LPF_APS2[i+1] = LPF_APS2[i];
    LPF_BPS1[i+1] = LPF_BPS1[i];
    LPF_BPS2[i+1] = LPF_BPS2[i];
  }
  // Read new current sensor values:
  LPF_APS1[0] = analogRead(APS1_IN);      
  LPF_APS2[0] = analogRead(APS2_IN);
  LPF_BPS1[0] = analogRead(BPS1_IN);
  LPF_BPS1[0] = analogRead(BPS2_IN);
  // Add new value to average:
  AV_APS1 += LPF_APS1[0];               
  AV_APS2 += LPF_APS2[0];
  AV_BPS1 += LPF_BPS1[0];
  AV_BPS2 += LPF_BPS2[0];
  // Divide sum to find average value:
  AV_APS1 /= BUFFERSIZE;  
  AV_APS2 /= BUFFERSIZE;
  AV_BPS1 /= BUFFERSIZE;
  AV_BPS2 /= BUFFERSIZE;
  // Map to fix slope (and make sure not to create DIV/0 errors in the process)
  AV_APS1 = map(AV_APS1-APS1_OFFSET, APS1_LOW, APS1_HIGH, 1, 1024)
  AV_APS2 = map(AV_APS2-APS2_OFFSET, APS2_LOW, APS2_HIGH, 1024, 1)
  AV_BPS1 = map(AV_BPS1-BPS1_OFFSET, BPS1_LOW, BPS1_HIGH, 1, 1024)
  AV_BPS2 = map(AV_BPS2-BPS2_OFFSET, BPS2_LOW, BPS2_HIGH, 1024, 1)
  // Check for signal shorts to GND or 5V
  if( AV_APS1 < 1 || AV_APS1 > 1024) STATUS = 11;
  if( AV_APS2 < 1 || AV_APS2 > 1024) STATUS = 12;
  if( AV_BPS1 < 1 || AV_BPS1 > 1024) STATUS = 21;
  if( AV_BPS2 < 1 || AV_BPS2 > 1024) STATUS = 22;
 
  // Check for signal plausibility
  if( plausibility_check(AV_APS1, AV_APS2)){
    THROTTLE = 0;
    BRAKE = 0;
    STATUS = 30;
  }
  if( plausibility_check(AV_BPS1, AV_BPS2)){
    THROTTLE = 0;
    BRAKE = 0;
    STATUS = 40;
  } 

 
  //Map to PWM range (0-255) and constrain
  THROTTLE = map( (AV_APS1+AV_APS2)/2, 1, 1024, 0, 255);
  THROTTLE = constrain(THROTTLE, 0, 255);
  BRAKE = map( (AV_BPS1+AV_BPS2)/2, 1, 1024, 0, 255);
  BRAKE = constrain(BRAKE, 0, 255);


  // Prevent BSPD from triggering
  // - 5kW from 66kW is 7.5%, so 19 of 255.
  // - 50% of brake is considered 'hard braking' for now.
  if (THROTTLE >= 19 && BRAKE >= 127) {
    THROTTLE = 0;
    BRAKE = 0;    
    STATUS = 50;    
    }
  
  // If time since last packet transmission > DATA_RATE, send new packet:
  if(millis() - last > DATA_RATE){
    Serial.print("Throttle setting is: ")
    Serial.println(THROTTLE);
    Serial.print("Brake setting is: ")
    Serial.println(BRAKE);
    Serial.print("Error state is: ")
    Serial.println(STATUS)

    CAN.beginPacket(0x12);
    CAN.write(THROTTLE);
    CAN.write(BRAKE);
    CAN.write(STATUS);
    CAN.endPacket();
    // Update timer:
    last=millis();
  }
}

bool plausibility_check(long POT1, long POT2) {

  bool IMPLAUSIBLE = true;

  float RATIO = (float)POT1 / (float)POT2;
  float DIFF = abs(RATIO - POT_RATIO);

  if (DIFF > MAX_DIFF) {
    IMPLAUSIBLE = true;
  } else {
    IMPLAUSIBLE = false;
  }
  return IMPLAUSIBLE;
}

