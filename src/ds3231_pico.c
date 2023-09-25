#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "tusb.h"

#include "ds3231_pico.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments

#define I2C_INSTANCE i2c0
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Single register read logic
static bool _ds3231_read_register(ds3231_inst_t *inst, uint8_t *reg, uint8_t *data);
void gpio_event_string(char *buf, uint32_t events);

struct ds3231_inst {
	i2c_inst_t *i2c;
};

ds3231_inst_t *ds3231_create(void) {
	return malloc(sizeof (ds3231_inst_t));
}

void ds3231_destroy(ds3231_inst_t *inst) {
	free(inst);
}

bool ds3231_init(ds3231_inst_t *inst, i2c_inst_t *i2c) {
	if (inst == NULL) return false;
	inst->i2c = i2c;

	return true;
}

bool ds3231_datetime_get(ds3231_inst_t *inst, ds3231_datetime_t *dt) {
	if (inst == NULL) return false;

	uint8_t address = 0x00;
	uint8_t data[7];

	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, &address, 1, true);
	if (rval == PICO_ERROR_GENERIC) return false;
	rval = i2c_read_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 7, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	dt->seconds = (data[0] & 0x0F) + ((data[0] >> 4) & 0x07) * 10;

	dt->minutes = (data[1] & 0x0F) + ((data[1] >> 4) & 0x07) * 10;

	dt->hour_mode = data[2] & 0x40;

	if (dt->hour_mode) 
		dt->post_meridiem = !!(data[2] & 0x20);
	else 
		dt->post_meridiem = dt->hours >= 12;

	uint8_t mask = dt->hour_mode ? 0x01 : 0x03;
	dt->hours = (data[2] & 0x0F) + ((data[2] >> 4) & mask) * 10;

	dt->day = data[3] & 0x07;

	dt->date = (data[4] & 0x0F) + ((data[4] >> 4) & 0x03) * 10;

	dt->century = !!(data[5] & 0x80);

	dt->month = (data[5] & 0x0F) + ((data[5] >> 4) & 0x01) * 10;

	dt->year = (data[6] & 0x0F) + ((data[6] >> 4) & 0x01) * 10;

	return true;
}

bool ds3231_datetime_set(ds3231_inst_t *inst, ds3231_datetime_t *dt) {
	if (inst == NULL) return false;
	uint8_t data[8] = {0x00};

	data[1] |= (dt->seconds % 10) | ((dt->seconds / 10) << 4);
	data[2] |= (dt->minutes % 10) | ((dt->minutes / 10) << 4);

	if (dt->hour_mode) {
		data[3] = 0x40;
		if (dt->post_meridiem) data[3] |= 0x20;	
	}

	data[3] |= (dt->hours % 10) | ((dt->hours / 10) << 4);
	data[4] = dt->day;

	data[5] |= (dt->date % 10) | ((dt->date / 10) << 4);

	if (dt->century) data[6] = 0x80;
	data[6] |= (dt->month % 10) | ((dt->month / 10) << 4);

	data[7] |= (dt->year % 10) | ((dt->year / 10) << 4);

	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 8, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	return true;
}

bool ds3231_alarm1_set(ds3231_inst_t *inst, ds3231_datetime_t *dt, alarm1_mode_t mode) {
	if (inst == NULL) return false;

	// data[0] = first alarm reg
	uint8_t data[5] = {DS3231_REG_ALARM1_SECS, 0x00};

	if (mode & 0x01) data[1] = 0x80;
	if (mode & 0x02) data[2] = 0x80;
	if (mode & 0x04) data[3] = 0x80;
	if (mode & 0x08) data[4] = 0x80;

	// DAY mode set
	if (mode & 0x10) data[4] |= 0x40;

	switch (mode) {
	case ALARM1_MATCH_DHMS_DATE:
	case ALARM1_MATCH_DHMS_DAY:
		if (mode & 0x10)
			data[4] |= dt->day;
		else
			data[4] |= (dt->date % 10) | ((dt->date / 10) << 4);

	case ALARM1_MATCH_HMS:
		if (dt->hour_mode) {
			data[3] |= 0x40;
			if (dt->post_meridiem) data[3] |= 0x20;
		}

		data[3] |= (dt->hours % 10) | ((dt->hours / 10) << 4);

	case ALARM1_MATCH_MS:
		data[2] |= (dt->minutes % 10) | ((dt->minutes / 10) << 4);

	case ALARM1_MATCH_S:
		data[1] |= (dt->seconds % 10) | ((dt->seconds / 10) << 4);
	}

	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 5, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	return true;
}

bool ds3231_alarm2_set(ds3231_inst_t *inst, ds3231_datetime_t *dt, alarm2_mode_t mode) {
	if (inst == NULL) return false;

	// data[0] = first alarm reg
	uint8_t data[4] = {DS3231_REG_ALARM2_MINS, 0x00};

	if (mode & 0x01) data[1] = 0x80;
	if (mode & 0x02) data[2] = 0x80;
	if (mode & 0x04) data[3] = 0x80;

	// DAY mode set
	if (mode & 0x08) data[3] |= 0x40;

	switch (mode) {
	case ALARM2_MATCH_DHM_DATE:
	case ALARM2_MATCH_DHM_DAY:
		if (mode & 0x08)
			data[3] |= dt->day;
		else
			data[3] |= (dt->date % 10) | ((dt->date / 10) << 4);

	case ALARM2_MATCH_HM:
		if (dt->hour_mode) {
			data[2] |= 0x40;
			if (dt->post_meridiem) data[2] |= 0x20;
		}

		data[2] |= (dt->hours % 10) | ((dt->hours / 10) << 4);

	case ALARM2_MATCH_M:
		data[1] |= (dt->minutes % 10) | ((dt->minutes / 10) << 4);
	}

	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 4, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	return true;
}

bool ds3231_alarm_irq_enable(ds3231_inst_t *inst, uint alarm) {
	if (inst == NULL) return false;
	if (alarm != 1 && alarm != 2) return false;

	uint8_t data[2] = {DS3231_REG_CTRL, 0x00};

	if (!_ds3231_read_register(inst, &data[0], &data[1]))
		return false;

	data[1] |= alarm;

	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 2, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	return true;
}

bool ds3231_alarm_irq_disable(ds3231_inst_t *inst, uint alarm) {
	if (inst == NULL) return false;
	if (alarm != 1 && alarm != 2) return false;

	uint8_t data[2] = {DS3231_REG_CTRL, 0x00};

	if (!_ds3231_read_register(inst, &data[0], &data[1]))
		return false;

	data[1] &= ~alarm;

	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 2, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	return true;
}

bool ds3231_alarm_clear(ds3231_inst_t *inst, uint alarm) {
	if (inst == NULL) return false;
	if (alarm != 1 && alarm != 2) return false;

	uint8_t data[2] = {DS3231_REG_CTRL_STATUS, 0x00};

	if (!_ds3231_read_register(inst, &data[0], &data[1]))
		return false;

	data[1] &= ~alarm;

	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 2, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	return true;
}

bool ds3231_alarm_state(ds3231_inst_t *inst, uint alarm) {
	if (inst == NULL) return false;
	if (alarm != 1 && alarm != 2) return false;

	uint8_t data[2] = {DS3231_REG_CTRL_STATUS, 0x00};

	if (!_ds3231_read_register(inst, &data[0], &data[1]))
		return false;

	return data[1] & alarm;
}

static bool _ds3231_read_register(ds3231_inst_t *inst, uint8_t *reg, uint8_t *data) {
	
	int rval = i2c_write_blocking(inst->i2c, DS3231_I2C_ADDRESS, reg, 1, true);
	if (rval == PICO_ERROR_GENERIC) return false;
	rval = i2c_read_blocking(inst->i2c, DS3231_I2C_ADDRESS, data, 1, false);
	if (rval == PICO_ERROR_GENERIC) return false;

	return true;
}

void ds3231_datetime_print(ds3231_datetime_t *dt) {

	printf("secs: %u\n", dt->seconds);
	printf("mins: %u\n", dt->minutes);
	printf("mode: %s\n", dt->hour_mode ? "24" : "12");
	printf("hours: %u\n", dt->hours);
	printf("day: %u\n", dt->day);
	printf("date: %u\n", dt->date);
	printf("century: %u\n", dt->century);
	printf("month: %u\n", dt->month);
	printf("year: %u\n", dt->year);
	
}

int ds3231_datetime_compare(ds3231_datetime_t *a, ds3231_datetime_t *b) {

	int compare = b->century - a->century;
	if (compare) return compare;

	compare = b->year - a->year;
	if (compare) return compare;

	compare = b->month - a->month;
	if (compare) return compare;

	compare = b->date - a->date;
	if (compare) return compare;

	compare = b->day - a->day;
	if (compare) return compare;

	compare = b->hours - a->hours;
	if (compare) return compare;

	compare = b->minutes - a->minutes;
	if (compare) return compare;

	return b->seconds - a->seconds;
}

void gpio_callback(uint gpio, uint32_t events) {
	char event_str[128];
	gpio_event_string(event_str, events);
	printf("%d %s\n", gpio, event_str);
	printf("gpio4: %u\n", gpio_get(4));
}

//int main() {
//    // Set Picotool binary info
//    stdio_init_all(); // To be able to use printf
//					  
//	// Wait until usb serial is connected
//	while (!tud_cdc_connected()) { sleep_ms(100); };
//
//	// Initialize I2C
//	i2c_init(I2C_INSTANCE, 100 * 4000);	
//	gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
//	gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
//	gpio_pull_up(I2C_SDA_PIN);
//	gpio_pull_up(I2C_SCL_PIN);
//
//	uint8_t data[2] = {0x00};
//
//	ds3231_inst_t *ds3231 = ds3231_create();
//	ds3231_init(ds3231, I2C_INSTANCE);
//
//	ds3231_datetime_t dt;
//	ds3231_datetime_get(ds3231, &dt);
//	dt.minutes = 59;
//
//	ds3231_datetime_set(ds3231, &dt);
//	ds3231_alarm_irq_enable(ds3231, 1);
//
//    gpio_init(4);
//    gpio_set_dir(4, GPIO_IN);
//	gpio_set_irq_enabled_with_callback(5, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
//
//	uint count = 0;
//	bool reset = true;
//	for (;;) {
//
//		if (reset) {
//			//printf("Alarm! Resetting...\n");
//
//			ds3231_datetime_get(ds3231, &dt);
//
//			if (dt.minutes < 59)
//				dt.minutes += 1;
//			else
//				dt.minutes = 0;
//
//			ds3231_alarm1_set(ds3231, &dt, ALARM1_EVERY_SECOND);
//
//			ds3231_alarm_clear(ds3231, 1);
//			count = 0;
//			reset = false;
//		} else {
//			//printf("%u\n", ++count);
//		}
//
//		sleep_ms(500);
//		reset = ds3231_alarm_state(ds3231, 1);
//	}
//    
//    return 0;
//}

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
