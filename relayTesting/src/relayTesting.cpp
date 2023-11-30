/* 
 * Project: Relay tester, open and close a relay for measuring purposes
 * Author: Rebecca Snyder
 * Date: 2023-11-30
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */


#include "Particle.h"
// use Emitter Follower configuration to run relay off 5v on Particle Photon 2
// Emitter Follower built with 2N3906, 220 ohm and 2.2 kOhm resistor
const int RELAYPIN = D10;

SYSTEM_MODE(SEMI_AUTOMATIC);



void setup() {
  Serial.begin(9600);
  pinMode(RELAYPIN,OUTPUT);
}

void loop() {
  digitalWrite(RELAYPIN,HIGH);
  Serial.printf("pin high / relay closed / on\n");
  delay(5000);
  digitalWrite(RELAYPIN,LOW);
  Serial.printf("relay off\n");
  delay(5000);
}
