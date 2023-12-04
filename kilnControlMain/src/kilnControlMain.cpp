/* 
 * Project: Kiln Controller 
 * Author: Rebecca Snyder
 * Date: 2023-11-30
 * 
 * 
 *  use ctrl-shift-p to install "MAX6675_CNM", particle library for thermocouple board 
 *  install "Adafruit_SSD1306"
 *  DO NOT install "Adafruit_GFX" separately  
 *  install "IoTClassroom_CNM"
 * 
 * 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 * 
 * 
 * AFTER LUNCH - IOT timer for hold?
 * -still need total time record for $ calc
 * -test with relay, thermo, maybe a "firiing sched" w/ hot water
 * 
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "MAX6675.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "IoTTimer.h"

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

const int numberOfSegments = 5; // conventionally numbering starts at 1
// glass full fuse sample firing schedule, source: https://www.bullseyeglass.com/wp-content/uploads/2023/02/technotes_04.pdf
//    for two layers of 3mm (1/8") thickness, 12" diam with top and side elements
int firingSchedule [numberOfSegments] [3] = {
  // DPH (degrees F per hour), target temp in F, hold time in minutes
  // USE NEGATIVE DPH FOR TEMP REDUCTIONS
  {400, 1250, 30}, // initial heat and rapid soak
  {600, 1490, 10}, // rapid heat process soak
  {-AFAP, 900, 30}, // rapid cool anneal soak
  {-150, 700, 0},  // anneal cool
  {-AFAP, 70, 0}
};

int currentSegment;
unsigned long timeSegmentStart;
int targetTemp;
int currentTemp;
unsigned long timeFiringRunStarts;
double targetSlope;
double currentSlope;
int i;
bool relayClosed;
unsigned long timeRelayClosed;




int kilnFarenheitEndActual [numberOfSegments]; //will want the temp of ending time
unsigned long kilnTimeStartActul [numberOfSegments];
unsigned long holdTimeStartActual [numberOfSegments];
unsigned long kilnAccumulatedTimePowered [numberOfSegments]; //track time on to calculate electric usage








SYSTEM_MODE(SEMI_AUTOMATIC);
void displayTemperatures(float fahrenheit, float celsius);



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

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  thermocoupleStatus = thermocouple.read();
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
  displayTemperatures(tempF,tempC);
  delay(2000);
  
  timeFiringRunStarts = millis();
  //kiln loop
  for (i = 0; i <= numberOfSegments; i++) {
    currentSegment = i + 1;
    timeSegmentStart = millis();
    targetTemp = (firingSchedule [i][1]);
    targetSlope = (firingSchedule [i][0]);
    tempC = thermocouple.getTemperature();
    tempF = (9.0/5.0)* tempC + 32;
    displayTemperatures(tempF,tempC);
    delay(1000);
    while (tempF != targetTemp) {
      tempC = thermocouple.getTemperature();
      tempF = (9.0/5.0)* tempC + 32;
      currentSlope = (targetTemp - tempC) / (millis()-timeSegmentStart);
      kilnAccumulatedTimePowered[i]=0;
      if (currentSlope <= targetSlope) {
        digitalWrite(RELAYPIN,HIGH);
        //relayClosed = true; 
        timeRelayClosed = millis(); //TIME ACCUMUL NEEDS WORK
        Serial.printf("relay ON, segment %d, curr %f, target %d \n", currentSegment, tempF, targetTemp);
      }
      else {
        digitalWrite(RELAYPIN,LOW); // turn off relay if slope greater than desired
        //relayClosed = false;
        kilnAccumulatedTimePowered[i] = (millis()-timeRelayClosed) + kilnAccumulatedTimePowered[i];
      }
    } 
    holdTimeStartActual[i] = millis();
    holdTimeTimer.startTimer((firingSchedule[i][2])*60000);
    //if (millis() <= (holdTimeStartActual[i] + ((firingSchedule[i][2])*60000))) {
    if (!holdTimeTimer.isTimerReady()) {
      tempC = thermocouple.getTemperature();
      tempF = (9.0/5.0)* tempC + 32;
      if (tempF <= targetTemp) {
        digitalWrite(RELAYPIN,HIGH);
        timeRelayClosed = millis();
        Serial.printf("hold & heat \n");
      }
      else {
        digitalWrite(RELAYPIN,LOW); // turn off relay if slope greater than desired
        kilnAccumulatedTimePowered[i] = (millis()-timeRelayClosed) + kilnAccumulatedTimePowered[i];
        Serial.printf("hold & relay off \n");
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
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.printf("segment %d \n",currentSegment);
  display.printf("target %d F, at %f per hr \n", targetTemp, targetSlope);
  display.display();
}







