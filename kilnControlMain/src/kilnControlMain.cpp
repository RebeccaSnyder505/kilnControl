/* 
 * Project: Kiln Controller 
 * Author: Rebecca Snyder
 * Date: 2023-11-30
 * 
 * 
 *  use ctrl-shift-p to install "MAX6675_CNM", particle library for thermocouple board 
 *  install "Adafruit_SSD1306"
 *  DO NOT install "Adafruit_GFX" separately  
 * 
 * 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "MAX6675.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#define AFAP 50000 // "As Fast As Possible" -- use more degrees than is possible

//thermocouple
MAX6675 thermocouple;
byte thermocoupleStatus;
float tempC, tempF;

//Adafruit OLED
#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);

const int numberOfSegments = 5; // conventionally numbering starts at 1
// glass full fuse sample firing schedule, source: https://www.bullseyeglass.com/wp-content/uploads/2023/02/technotes_04.pdf
//    for two layers of 3mm (1/8") thickness, 12" diam with top and side elements
int firingSchedule1 [numberOfSegments] [3] = {
  // DPH (degrees F per hour), target temp in F, hold time in minutes
  {400, 1250, 30}, // initial heat and rapid soak
  {600, 1490, 10}, // rapid heat process soak
  {AFAP, 900, 30}, // rapid cool anneal soak
  {150, 700, 0},  // anneal cool
  {AFAP, 70, 0}
};

int currentSegment;
int TargetTemp;

//kiln temperature related
int kilnFarenheitEndDataActual [numberOfSegments]; //will want the temp of ending time
int kilnTimeStartDataActual [numberOfSegments]; //will want the "start" of ending time

int currentTempKiln; // current temp in fahrenheit
int timeSegmentStarted; 
float tempDifferenceFromSegmentStart;
double targetSlope;
double currentSlope;

// note, will need to account for this somehow
//target temperature, "reached" should be temp remaining in range






SYSTEM_MODE(SEMI_AUTOMATIC);
void displayTemperatures(float fahrenheit, float celsius);



void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);

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
    Serial.printf("thermocouple %d",thermocoupleStatus);
  }
  Serial.printf("raw data %d \n", thermocouple.getRawData());
  tempC = thermocouple.getTemperature();
  tempF = (9.0/5.0)* tempC + 32;
  Serial.printf("Temperature:%0.2f or %0.2f\n",tempF,tempC);
  
  //OLED
  displayTemperatures(tempF,tempC);
  delay(2000);


}

void displayTemperatures(float fahrenheit, float celsius) {
  display.clearDisplay();   // clears the screen and buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("F %f \n",fahrenheit);
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.printf("C %f \n",celsius); 
  display.display();
}

