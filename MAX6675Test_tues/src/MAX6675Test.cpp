/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

#include "SPI.h"
#include "MAX6675.h"

const int MAXDO = D16; //7 // Defining the MISO pin
const int MAXCS = D18; //6 // Defining the CS pin
const int MAXCLK = D17; //5 // Defining the SCK pin


MAX6675 thermocouple;


void setup ()
{
  Serial.begin(9600);
  waitFor(Serial.isConnected, 10000);


  thermocouple.begin(MAXCS, MAXCS, MAXDO);
}


void loop () {
  int status = thermocouple.read();
  if (status != STATUS_OK)
  {
    Serial.println("ERROR!");
  }

  uint32_t value = thermocouple.getRawData();  // Read the raw Data value from the module
  Serial.print("RAW:\t");

  // Display the raw data value in BIN format
  uint32_t mask = 0x80000000; 
  for (int i = 0; i < 32; i++) 
  {
    if ((i > 0)  && (i % 4 == 0)) Serial.print("-");
    Serial.print((value & mask) ? 1 : 0);
    mask >>= 1;
  }
  Serial.println();

  Serial.print("TMP:\t");
  Serial.println(thermocouple.getTemperature(), 3);

  delay(100);
}


// -- END OF FILE --