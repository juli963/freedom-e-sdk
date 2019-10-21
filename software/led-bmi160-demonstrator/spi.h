typedef struct SPIMaster {
    uint32_t cs;
} SPIMaster;

int8_t spi_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
int8_t spi_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
void spi_begin(uint32_t cs, uint32_t max_slave_freq);
uint8_t reg_read_bits(uint8_t reg, unsigned pos, unsigned len);
