#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <CayenneLPP.h>
#include <DHT.h>

// DHT Sensor Setup
#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Cayenne LPP (Low Power Payload)
CayenneLPP lpp(51);

// LoRaWAN Session Keys (ABP Mode)
static const PROGMEM u1_t NWKSKEY[16]  = { 0x8A, 0x59, 0x27, 0x24, 0x20, 0xC7, 0x63, 0x0A, 0x57, 0xE9, 0x52, 0x9E, 0x33, 0x99, 0xD4, 0x6F };
static const PROGMEM u1_t APPSKEY[16]  = { 0x82, 0x3C, 0x99, 0xF4, 0x78, 0xB2, 0x2F, 0xBE, 0xF0, 0x26, 0xCF, 0x51, 0x26, 0x0E, 0xF8, 0x6E };
static const u4_t DEVADDR = 0x26011EAA;

// LoRa Pin Mapping (Arduino UNO Example)
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = {2, 3, LMIC_UNUSED_PIN}
};

static osjob_t sendjob;
const unsigned TX_INTERVAL = 10; // seconds
int compteur = 0;

// OTAA Callbacks (unused for ABP)
void os_getArtEui(u1_t* buf) {}
void os_getDevEui(u1_t* buf) {}
void os_getDevKey(u1_t* buf) {}

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");

  switch (ev) {
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));

      if (LMIC.txrxFlags & TXRX_ACK) {
        Serial.println(F("Received ack"));
      }

      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload:"));
        
        for (int i = 0; i < LMIC.dataLen; i++) {
          uint8_t b = LMIC.frame[LMIC.dataBeg + i];
          if (b < 0x10) Serial.print(F("0"));
          Serial.print(b, HEX);
        }
        Serial.println();

        // Basic downlink payload handling (index 2)
        uint8_t cmd = LMIC.frame[LMIC.dataBeg + 2];
        if (cmd == 0x00) digitalWrite(13, HIGH);
        else if (cmd == 0x67) digitalWrite(13, LOW);
      }

      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;

    // Other LMIC events
    case EV_JOINED:         Serial.println(F("EV_JOINED")); break;
    case EV_JOINING:        Serial.println(F("EV_JOINING")); break;
    case EV_JOIN_FAILED:    Serial.println(F("EV_JOIN_FAILED")); break;
    case EV_RESET:          Serial.println(F("EV_RESET")); break;
    case EV_RXCOMPLETE:     Serial.println(F("EV_RXCOMPLETE")); break;
    case EV_LINK_ALIVE:     Serial.println(F("EV_LINK_ALIVE")); break;
    case EV_LINK_DEAD:      Serial.println(F("EV_LINK_DEAD")); break;
    case EV_SCAN_TIMEOUT:   Serial.println(F("EV_SCAN_TIMEOUT")); break;
    case EV_BEACON_FOUND:   Serial.println(F("EV_BEACON_FOUND")); break;
    case EV_BEACON_MISSED:  Serial.println(F("EV_BEACON_MISSED")); break;
    case EV_BEACON_TRACKED: Serial.println(F("EV_BEACON_TRACKED")); break;
    default:                Serial.println(F("Unknown event")); break;
  }
}

void do_send(osjob_t* j) {
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
    return;
  }

  compteur++;

  if (compteur == 1) {
    int h = dht.readHumidity();
    Serial.print(F("Humidity: "));
    Serial.println(h);
    lpp.reset();
    lpp.addRelativeHumidity(1, h);
    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
  } 
  else if (compteur == 2) {
    int t = dht.readTemperature();
    Serial.print(F("Temperature: "));
    Serial.println(t);
    lpp.reset();
    lpp.addTemperature(1, t);
    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
    compteur = 0;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Starting LoRa DHT11 LPP Node"));

  dht.begin();
  pinMode(13, OUTPUT);

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
#elif defined(CFG_us915)
  LMIC_selectSubBand(1);
#endif

  LMIC_setLinkCheckMode(0);
  LMIC.dn2Dr = DR_SF9;
  LMIC_setDrTxpow(DR_SF7, 14);
  LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);

  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
