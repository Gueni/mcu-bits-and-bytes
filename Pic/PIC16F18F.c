#include <xc.h>

#define _XTAL_FREQ 8000000
#define CS LATBbits.LATB0

void SPI_Init() {
    TRISC5 = 0; // SDO output
    TRISC3 = 0; // SCK output
    TRISC4 = 1; // SDI input
    TRISB0 = 0; // CS

    SSPSTAT = 0x40; // CKE=1
    SSPCON = 0x20;  // Enable SPI, FOSC/4
    CS = 1;
}

void SPI_WriteRegister(uint8_t addr, uint8_t val) {
    CS = 0;
    SSPBUF = addr | 0x80;
    while(!SSPSTATbits.BF);
    SSPBUF = val;
    while(!SSPSTATbits.BF);
    CS = 1;
}

uint8_t SPI_ReadRegister(uint8_t addr) {
    uint8_t val;
    CS = 0;
    SSPBUF = addr & 0x7F;
    while(!SSPSTATbits.BF);
    SSPBUF = 0x00; // dummy write
    while(!SSPSTATbits.BF);
    val = SSPBUF;
    CS = 1;
    return val;
}
