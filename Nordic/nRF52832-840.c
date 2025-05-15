#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "boards.h"

#define SPI_INSTANCE  0
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
static uint8_t m_rx_buf[2];

void spi_init(void) {
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, NULL, NULL));
}

uint8_t spi_read_register(uint8_t addr) {
    uint8_t tx[2] = { addr & 0x7F, 0x00 };
    nrf_drv_spi_transfer(&spi, tx, 2, m_rx_buf, 2);
    return m_rx_buf[1];
}

void spi_write_register(uint8_t addr, uint8_t val) {
    uint8_t tx[2] = { addr | 0x80, val };
    nrf_drv_spi_transfer(&spi, tx, 2, NULL, 0);
}
