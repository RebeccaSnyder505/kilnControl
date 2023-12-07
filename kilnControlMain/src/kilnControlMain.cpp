/* 
 * Project: Kiln Controller 
 * Author: Rebecca Snyder
 * Date: 2023-11-30
 * 
 * Control elaborate heat/cool schedules for a glass kiln, 
 * can specify  -degrees F per hour for each segment
 *              -target temperature for each segment
 *              -hold time for each segment
 * authorized user must login to https://io.adafruit.com/ to start the schedule             
 * 
 * 
 * 
 * 
 *  use ctrl-shift-p to install "MAX6675_CNM", particle library for thermocouple board 
 *  install "Adafruit_SSD1306"
 *  DO NOT install "Adafruit_GFX" separately  
 *  install "IoTClassroom_CNM"
 *  install "Adafruit_MQTT"
 * 
 * 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 * 
 * 
 * For tues - IOT timer for hold?
 * -still need total time record for $ calc
 * -might try approaching heating/cooling/maintaining with switch case
 * 
 */


#include "Particle.h"
#include "MAX6675.h"  // thermocouple board
#include "Adafruit_GFX.h" 
#include "Adafruit_SSD1306.h" // OLED screen
#include "IoTTimer.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"


#define AFAP 50000 // "As Fast As Possible" -- use more degrees than is possible


//thermocouple
MAX6675 thermocouple;
byte thermocoupleStatus;
float tempC, tempF;
const int RELAYPIN = D10;

//Adafruit OLED
#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);

//IoTTimer
IoTTimer holdTimeTimer;

// RATE/TARGET/HOLD model
const int numberOfSegments = 5; // conventionally numbering starts at 1
// glass full fuse sample firing schedule, source: https://www.bullseyeglass.com/wp-content/uploads/2023/02/technotes_04.pdf
//    for two layers of 3mm (1/8") thickness, 12" diam with top and side elements
// int firingSchedule [numberOfSegments] [3] = {
//   // DPH (degrees F per hour), target temp in F, hold time in minutes
//   // USE NEGATIVE DPH FOR TEMP REDUCTIONS
//   {400, 1250, 30}, // initial heat and rapid soak
//   {600, 1490, 10}, // rapid heat process soak
//   {-AFAP, 900, 30}, // rapid cool anneal soak
//   {-150, 700, 0},  // anneal cool
//   {-AFAP, 70, 0}
// };

//FOR KETTLE, for testing
// RATE/TARGET/HOLD MODEL
// int firingSchedule [numberOfSegments] [3] = { //fake schedule to run kettle
//   // DPH (degrees F per hour), target temp in F, hold time in minutes
//   // USE NEGATIVE DPH FOR TEMP REDUCTIONS
//   {AFAP, 180, 0}, // heat quickly to a nice temp for green tea
//   {10, 200, 10}, // slowly heat for other tea types
//   {0, 200, 30}, // what happens at slope 0 when holding temp?
//   {10, 100, 30},  // slow cool to 100F
//   {-AFAP, 0, 0} // cool AFAP to zero
// };

//THE HEAT/COOL/MAINTAIN model, tuesday
// {rate to reach target temp, target}
int firingSchedule [numberOfSegments] [3] = { //fake schedule to run kettle
  // DPH (degrees F per hour), target temp in F, hold time in minutes
  {AFAP, 120, 0}, // heat quickly to a nice temp for testing
  {-10, 150, 2}, // slowly cool then 5 min hold
  {0, 100, 2}, //  hold time -- maybe "rate" for hold time needs 0 ?
  {-10, 90, 2},  // slow cool
  {-AFAP, 0, 0} // cool AFAP to zero
};
int behavior;

int currentSegment;
unsigned long timeSegmentStart;
float tempSegmentStart;
int targetTemp;
unsigned long timeFiringRunStarts;
double targetSlope;
double currentSlope;
int i;
bool relayClosed;
unsigned long timeRelayClosed;
bool adafruitKilnButton;






int kilnFarenheitEndActual [numberOfSegments]; //will want the temp of ending time
unsigned long kilnTimeStartActul [numberOfSegments];
unsigned long holdTimeStartActual [numberOfSegments];
unsigned long kilnAccumulatedTimePowered [numberOfSegments]; //track time on to calculate electric usage


void displayTemperatures(float fahrenheit, float celsius);

// to work with AdaFruit
unsigned long prevAdafruitTime;
const int timeIntervalAdafruit = 10000; 
unsigned long updateAdafruitTemp(float tempF,unsigned long prevAdafruitTime,int timeIntervalAdafruit); // function for tempF to adafruit



// the following is AdaFruit MQTT setup 
/************ Global State (you don't need to change this!) ***   ***************/ 
TCPClient TheClient; 

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 

Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

/****************************** Feeds ***************************************/ 
// Setup Feeds to publish or subscribe 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname> 

Adafruit_MQTT_Publish thermocoupleTempF = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/thermocoupletempf");
Adafruit_MQTT_Subscribe kilnButton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/kiln");

/************Declare Variables*************/
unsigned int last, lastTime;
float subValue,pubValue;

/************Declare Functions*************/
void MQTT_connect();
bool MQTT_ping();


//SYSTEM_MODE(AUTOMATIC);




void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);

  pinMode(RELAYPIN,OUTPUT);
  digitalWrite(RELAYPIN,LOW); //make sure relay is off for safety
  relayClosed = false;

  //thermocouple setup
  thermocouple.begin(SCK, SS, MISO);
  thermocoupleStatus = thermocouple.read();
  if (thermocoupleStatus != STATUS_OK) {
    Serial.printf("ERROR WITH THERMOCOUPLE!\n");
  }
  else {
    Serial.printf("thermocouple %d",thermocoupleStatus);
  }

  //OLED setup
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // display I am using has I2C addr 0x3C (for the 128x64)

  // Wifi setup
  // Connect to Internet but not Particle Cloud
  WiFi.on();
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  }
  Serial.printf("\n\n");

  // MQTT for AdaFruit
  prevAdafruitTime = millis();
  mqtt.subscribe(&kilnButton);
}



void loop() {
  MQTT_connect();
  MQTT_ping();
  thermocoupleStatus = thermocouple.read(); // do a read before each call for temp
  if (thermocoupleStatus != STATUS_OK) {
    Serial.printf("ERROR WITH THERMOCOUPLE!\n");
  }
  else {
    Serial.printf("thermocouple %d \n",thermocoupleStatus);
  }
  Serial.printf("raw data %d \n", thermocouple.getRawData());
  tempC = thermocouple.getTemperature();
  tempF = (9.0/5.0)* tempC + 32;
  Serial.printf("Temperature:%0.2f or %0.2f\n",tempF,tempC);
  
  //OLED
  adafruitKilnButton = FALSE;
  Serial.printf("Authorized user can login to toggle kiln cycle start\n");
  display.clearDisplay(); 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("%6.2f F \n",tempF);
  display.printf("%6.2f C \n",tempC); 
  display.printf("login via AdaFruit to start kiln cycle");
  display.display();
  //check Adafruit Button
  adafruitKilnButton = FALSE;
  while (!adafruitKilnButton) {
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(100))) { // the subscription part
      if (subscription == &kilnButton) {
        Serial.printf("if (subscription == &kilnButton) \n");
        subValue = atof((char *)kilnButton.lastread);
        Serial.printf("kiln toggled on AdaFruit %f \n", subValue);
        adafruitKilnButton = TRUE;
      }
    }
  }
  timeFiringRunStarts = millis();
  //kiln loop
  for (i = 0; i <= numberOfSegments; i++) { // going through the firing segments
    currentSegment = i + 1;
    timeSegmentStart = millis();
    targetTemp = (firingSchedule [i][1]);
    targetSlope = (firingSchedule [i][0]);
    currentSlope = targetSlope; // to give initial value for ramp up/down while loop
    //thermocouple.read();
   // tempC = thermocouple.getTemperature();   
    //tempF = (9.0/5.0)* tempC + 32;
    displayTemperatures(tempF,tempC);
    tempSegmentStart = tempF;
    timeSegmentStart = millis();
  
  // while loop for ramping up/down
    Serial.printf("about to start ramp up/down, part of segment %d\n",currentSegment);
    while ((currentSlope > 0 && tempF < targetTemp) || (currentSlope < 0 && tempF > targetTemp)) { 
      delay(5000);
      thermocouple.read();
      tempC = thermocouple.getTemperature();
      tempF = (9.0/5.0)* tempC + 32;
      currentSlope = ((tempSegmentStart - tempF) / (millis()-timeSegmentStart)) * 3600000.0; // 3.6 million milliseconds per hour
      tempSegmentStart = tempF;
      timeSegmentStart = millis();
      kilnAccumulatedTimePowered[i]=0;
      Serial.printf("current slope %g, target slope %g \n",currentSlope,targetSlope);
      //updateAdafruitTemp(tempF,prevAdafruitTime,timeIntervalAdafruit);
      if (currentSlope <= targetSlope) { 
        digitalWrite(RELAYPIN,HIGH);
        //relayClosed = true; 
        timeRelayClosed = millis(); //TIME ACCUMUL NEEDS WORK
        Serial.printf("relay ON, segment %d, curr %f, target %d \n", currentSegment, tempF, targetTemp);
      }
      else {
        digitalWrite(RELAYPIN,LOW); // turn off relay if slope greater than desired
        //relayClosed = false;
        // tracking the amount of time relay closed, this part needs work
        kilnAccumulatedTimePowered[i] = (millis()-timeRelayClosed) + kilnAccumulatedTimePowered[i];
      }
    } 
    holdTimeStartActual[i] = millis();
    holdTimeTimer.startTimer((firingSchedule[i][2])*60000.0);
    Serial.printf("about to start hold time, part of segment %d \n",currentSegment);
    //if (millis() <= (holdTimeStartActual[i] + ((firingSchedule[i][2])*60000))) {
    while (!holdTimeTimer.isTimerReady()) {
      delay(5000);
      thermocouple.read();
      tempC = thermocouple.getTemperature();
      tempF = (9.0/5.0)* tempC + 32;
      updateAdafruitTemp(tempF,prevAdafruitTime,timeIntervalAdafruit);
      if (tempF <= targetTemp) {
        digitalWrite(RELAYPIN,HIGH);
        timeRelayClosed = millis();
        Serial.printf("hold & heat \n");
        Serial.printf("relay ON, segment %d, curr %f, target %d \n", currentSegment, tempF, targetTemp);
      }
      else {
        digitalWrite(RELAYPIN,LOW); // turn off relay if slope greater than desired
        kilnAccumulatedTimePowered[i] = (millis()-timeRelayClosed) + kilnAccumulatedTimePowered[i];
        Serial.printf("hold & relay off \n");
        Serial.printf("relay OFF, segment %d, curr %f, target %d \n", currentSegment, tempF, targetTemp);
      }
    }
  }
}



void displayTemperatures(float fahrenheit, float celsius) { //display info on OLED
  display.clearDisplay();   // clears the screen and buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("F %f \n",fahrenheit);
  display.printf("C %f \n",celsius); 
  display.printf("segment %d \n",currentSegment);
  display.printf("target %d F, at %f.0 per hr \n", targetTemp, targetSlope);
  display.display();
}








void MQTT_connect() {
  int8_t ret;
  i = 0;
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
      i++;
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       if (i >= 6) {
        Serial.printf("MQTT not connecting for 30 seconds, giving up \n");
        return;
       }
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}


bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}



 unsigned long updateAdafruitTemp(float temp,unsigned long prevAdafruitTime,int timeIntervalAdafruit){
  MQTT_connect();
  MQTT_ping();
  Serial.printf("attempting adafruit update temp %f",temp);
  if((millis()-prevAdafruitTime*1.0) > timeIntervalAdafruit*1.0) {
    if(mqtt.Update()) {
      thermocoupleTempF.publish(temp);
    }
    prevAdafruitTime = millis();
  }
  return prevAdafruitTime;
}



 




