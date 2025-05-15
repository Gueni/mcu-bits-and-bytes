#include <SPI.h>
#define CS 10

void setup() {
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin();
}

void loop() {
  writeRegister(0x01, 0x55);
  delay(500);
}

void writeRegister(uint8_t reg, uint8_t val) {
  digitalWrite(CS, LOW);
  SPI.transfer(reg | 0x80);
  SPI.transfer(val);
  digitalWrite(CS, HIGH);
}
