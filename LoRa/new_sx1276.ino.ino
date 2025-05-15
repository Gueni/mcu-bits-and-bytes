#include <SPI.h>

// Chip Select (NSS) pin
const int nss = 8;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial to be ready (useful on boards like Leonardo)

  pinMode(nss, OUTPUT);
  digitalWrite(nss, HIGH); // Default HIGH (inactive)
  
  SPI.begin(); // Initialize SPI bus
}

void loop() {
  // Read a few registers
  readRegister(0x06);
  readRegister(0x07);
  readRegister(0x08);
  
  delay(2000); // Wait 2 seconds

  // Write to register 0x06
  writeRegister(0x06, 0xB9);

  // Read it back to confirm
  readRegister(0x06);
}

// Read 1 byte from register (7-bit address)
void readRegister(uint8_t address) {
  uint8_t response;

  digitalWrite(nss, LOW);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  
  SPI.transfer(address & 0x7F); // Clear MSB for read
  response = SPI.transfer(0x00); // Dummy write to receive response
  
  SPI.endTransaction();
  digitalWrite(nss, HIGH);

  Serial.print("Read 0x");
  Serial.print(address, HEX);
  Serial.print(": 0x");
  Serial.println(response, HEX);
}

// Write 1 byte to register (7-bit address)
void writeRegister(uint8_t address, uint8_t value) {
  digitalWrite(nss, LOW);
  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0)); // Faster SPI for write

  SPI.transfer(address | 0x80); // Set MSB for write
  SPI.transfer(value);

  SPI.endTransaction();
  digitalWrite(nss, HIGH);

  Serial.print("Wrote 0x");
  Serial.print(value, HEX);
  Serial.print(" to 0x");
  Serial.println(address, HEX);
}
