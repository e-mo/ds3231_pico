#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
//#include "tusb.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments

#define I2C_INSTANCE i2c1
#define I2C_ADDRESS 0x68
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

void set_bi() {
	// ADD BI HERE
}

void measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    //printf("pll_sys  = %dkHz\n", f_pll_sys);
    //printf("pll_usb  = %dkHz\n", f_pll_usb);
    //printf("rosc     = %dkHz\n", f_rosc);
    //printf("clk_sys  = %dkHz\n", f_clk_sys);
    //printf("clk_peri = %dkHz\n", f_clk_peri);
    //printf("clk_usb  = %dkHz\n", f_clk_usb);
    //printf("clk_adc  = %dkHz\n", f_clk_adc);
    //printf("clk_rtc  = %dkHz\n", f_clk_rtc);
    uart_default_tx_wait_blocking();
	sleep_ms(2000);
    // Can't measure clk_ref / xosc as it is the ref
}

void gpio_event_string(char *buf, uint32_t events);

void gpio_callback(uint gpio, uint32_t events) {
	uint8_t buf = 0x00;
	uint8_t data[2] = {0x0F};
	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[0], 1, true);
	i2c_read_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[1], 1, false);
	data[1] &= 0xFE;
	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, data, 2, true);

	static char event_str[128];
	gpio_event_string(event_str, events);
	//printf("GPIO %d %s\n", gpio, event_str);
}

int main() {
    // Set Picotool binary info
    set_bi();
    stdio_init_all(); // To be able to use printf
					  
	// Wait until usb serial is connected
	//while (!tud_cdc_connected()) { sleep_ms(100); };

	// Initialize I2C
	i2c_init(I2C_INSTANCE, 100 * 1000);	
	gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
	gpio_pull_up(I2C_SDA_PIN);
	gpio_pull_up(I2C_SCL_PIN);
	

	uint8_t mask = 0x80;
	uint8_t data[5] = {0};
	data[0] = 0x07;
	data[1] = 0x30;
	for (int i = 2; i <= 4; i++) {
		data[i] = mask;
	}

	//printf("%02X\n", data[1]);

	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, data, 5, true);

	uint8_t buf = 0x00;
	uint8_t address = 0x09;
	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[0], 1, true);
	i2c_read_blocking(I2C_INSTANCE, I2C_ADDRESS, &buf, 1, false);
	//printf("buf: %02X\n", buf);

	data[0] = 0x0E;
	data[1] = 0x1D;
	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, data, 2, false);

	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[0], 1, true);
	i2c_read_blocking(I2C_INSTANCE, I2C_ADDRESS, &buf, 1, false);
	//printf("buf: %02X\n", buf);

	//printf("buf: %02X\n", buf);
	//buf |= mask;
	//printf("after mask: %02X\n", buf);
	//i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, &address, 1, true);
	//i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, &buf, 1, false);

	//gpio_set_irq_enabled_with_callback(
	//		17, GPIO_IRQ_EDGE_FALL,
	//		true, &gpio_callback
	//);

	data[0] = 0x0F;
	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[0], 1, true);
	i2c_read_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[1], 1, false);
	data[1] &= 0xFE;
	i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, data, 2, true);


	gpio_init(PICO_DEFAULT_LED_PIN);
	gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

	// UART
	gpio_set_function(12, GPIO_FUNC_UART);
	gpio_set_function(13, GPIO_FUNC_UART);

	uint scb_orig = scb_hw->scr;
	uint clock0_orig = clocks_hw->sleep_en0;
	uint clock1_orig = clocks_hw->sleep_en1;
	for (;;) {
			
		sleep_run_from_xosc();
		sleep_goto_dormant_until_pin(17, true, false);

		rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
		scb_hw->scr = scb_orig;
		clocks_hw->sleep_en0 = clock0_orig;
		clocks_hw->sleep_en1 = clock1_orig;

		clocks_init();
		
		i2c_init(I2C_INSTANCE, 100 * 1000);	
		uart_init(uart0, 115200);
		uart_puts(uart0, "Still Alive!\n");
		sleep_ms(2000);

		data[0] = 0x0F;
		i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[0], 1, true);
		//printf("meow\n");
		i2c_read_blocking(I2C_INSTANCE, I2C_ADDRESS, &data[1], 1, false);

		data[1] &= 0xFE;
		i2c_write_blocking(I2C_INSTANCE, I2C_ADDRESS, data, 2, false);
		//printf("meow2\n");


		for (int i = 0; i < 30; i++) {
			gpio_put(PICO_DEFAULT_LED_PIN, 1);
			sleep_ms(100);
			gpio_put(PICO_DEFAULT_LED_PIN, 0);
			sleep_ms(100);
		}
	}

    
    return 0;
}

static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events add ", "
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}
