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

//thermocouple
MAX6675 thermocouple;
byte thermocoupleStatus;
float tempC, tempF;

//Adafruit OLED
#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);



SYSTEM_MODE(SEMI_AUTOMATIC);




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
  Serial.printf("Temperature:%0.2f or %0.2f\n",tempC,tempF);
  

   //OLED
  display.clearDisplay();   // clears the screen and buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("F %f \n",tempF);
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.printf("C %f \n",tempC);  display.setTextColor(WHITE);
  display.display();
  delay(2000);
}
 


