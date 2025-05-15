#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <CayenneLPP.h>
#include <DHT.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

// ================== DHT Sensor =====================
#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================== Cayenne LPP ====================
CayenneLPP lpp(51);

// ================== Watchdog & Sleep ==============
volatile int f_wdt = 1;
volatile int tempo = 0;

ISR(WDT_vect) {
  if (f_wdt == 0) {
    tempo++;
    f_wdt = 1;
  } else {
    Serial.println("WDT Overrun!!!");
  }
}

void enterSleep() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();     // CPU sleeps here
  sleep_disable();
  power_all_enable();
}

// ================== LoRa Configuration =============
static const PROGMEM u1_t NWKSKEY[16] = { 0x5F, 0x45, 0x16, 0x9B, 0x67, 0x9D, 0xCD, 0x52, 0x79, 0xC1, 0xCC, 0x3F, 0x8E, 0x63, 0xCA, 0xA0 };
static const PROGMEM u1_t APPSKEY[16] = { 0x9B, 0x83, 0x8A, 0x9F, 0xD1, 0x82, 0x0C, 0x8B, 0xF6, 0xED, 0x9F, 0xB8, 0x48, 0x72, 0xAF, 0xE5 };
static const u4_t DEVADDR = 0x2601133F;

void os_getArtEui(u1_t* buf) {}
void os_getDevEui(u1_t* buf) {}
void os_getDevKey(u1_t* buf) {}

static osjob_t sendjob;
const unsigned TX_INTERVAL = 10;

const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = { 2, 3, LMIC_UNUSED_PIN },
};

// ================== Event Handling =================
void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK) Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.print(F(" bytes: 0x"));

        for (int i = 0; i < LMIC.dataLen; i++) {
          uint8_t byte = LMIC.frame[LMIC.dataBeg + i];
          if (byte < 0x10) Serial.print("0");
          Serial.print(byte, HEX);

          // Optional LED control logic
          if (i == 2) {
            if (byte == 0x00) digitalWrite(13, HIGH);
            else if (byte == 0x67) digitalWrite(13, LOW);
          }
        }
        Serial.println();
      }
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;

    default:
      Serial.println(F("Unhandled event"));
      break;
  }
}

// ================== Transmission ===================
void do_send(osjob_t* j) {
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
    return;
  }

  int humidity = dht.readHumidity();
  Serial.print("Humidity: ");
  Serial.println(humidity);
  lpp.reset();
  lpp.addRelativeHumidity(1, humidity);
  LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);

  delay(1000); // Small delay between readings

  int temp = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.println(temp);
  lpp.reset();
  lpp.addTemperature(1, temp);
  LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
}

// ================== Setup ==========================
void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("Starting"));

  // Setup Watchdog
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = (1 << WDP0) | (1 << WDP3);  // 8s timeout
  WDTCSR |= _BV(WDIE);  // Enable interrupt

  // Pin init
  pinMode(13, OUTPUT);
  dht.begin();

  // LMIC init
  os_init();
  LMIC_reset();

  #ifdef PROGMEM
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);
  #else
    LMIC_setSession(0x1, DEVADDR, NWKSKEY, APPSKEY);
  #endif

  #if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
  #endif

  // Disable link check validation (automatically enabled)
  LMIC_setLinkCheckMode(0);

  // Set data rate and transmit power
  LMIC_setDrTxpow(DR_SF7, 14);

  // Start the first job
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
