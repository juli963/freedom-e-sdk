#include <stdio.h>
#include <mee/spi.h>
#include <mee/machine/sifive-hifive1.h>

int main() {
	printf("MEE SPI Driver Demo\n");

	mee_spi_init(&(__mee_dt_spi_10024000.spi), 100000);

	struct mee_spi_config config = {
		.protocol = SPI_SINGLE,
		.polarity = 0,
		.phase = 0,
		.little_endian = 0,
		.tx_only = 0,
		.cs_active_high = 0,
		.csid = 0,
		.baud_rate = 100000
	};

	char tx_buf[3] = {0x55, 0xAA, 0xA5};
	char rx_buf[3] = {0};

	mee_spi_transfer(&(__mee_dt_spi_10024000.spi), &config, 3, tx_buf, rx_buf);

	return 0;
}
