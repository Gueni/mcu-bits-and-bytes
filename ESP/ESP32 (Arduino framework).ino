#include <SPI.h>

#define NSS 5 // SPI CS pin

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, NSS); // SCK, MISO, MOSI, SS
  pinMode(NSS, OUTPUT);
  digitalWrite(NSS, HIGH);
}

void loop() {
  uint8_t regVal = readRegister(0x06);
  writeRegister(0x06, 0xB9);
  delay(1000);
}

uint8_t readRegister(uint8_t reg) {
  digitalWrite(NSS, LOW);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(reg & 0x7F); // Clear MSB for read
  uint8_t val = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(NSS, HIGH);
  return val;
}

void writeRegister(uint8_t reg, uint8_t val) {
  digitalWrite(NSS, LOW);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(reg | 0x80); // Set MSB for write
  SPI.transfer(val);
  SPI.endTransaction();
  digitalWrite(NSS, HIGH);
}
