/* DS3231 drivers for the RPi Pico. 
 *
 * Maintainer: Evan Morse
 * 			   tech@emorse.dev
 */
#ifndef DS3231_PICO_H
#define DS3231_PICO_H
// Time
#define DS3231_REG_SECS  	 	     0x00
#define DS3231_REG_MINS  		     0x01
#define DS3231_REG_HOUR  		     0x02
#define DS3231_REG_DAY   		     0x03
#define DS3231_REG_DATE  		     0x04
#define DS3231_REG_MONTH  		     0x05
#define DS3231_REG_YEAR  		     0x06
// Alarm 1
#define DS3231_REG_ALARM1_SECS       0x07
#define DS3231_REG_ALARM1_MINS       0x08
#define DS3231_REG_ALARM1_HOUR 	     0x09
#define DS3231_REG_ALARM1_DAY_DATE   0x0A
// Alarm 2
#define DS3231_REG_ALARM2_MINS       0x0B
#define DS3231_REG_ALARM2_HOUR       0x0C
#define DS3231_REG_ALARM2_DAY_DATE   0x0D
// Other
#define DS3231_REG_CTRL		         0x0E
#define DS3231_REG_CTRL_STATUS       0x0F
#define DS3231_REG_AGING_OFFSET      0x10
#define DS3231_REG_TEMP_MSB		     0x11
#define DS3231_REG_TEMP_LSB		     0x12

#define DS3231_I2C_ADDRESS 		     0x68

#include "pico/stdlib.h"
#include "hardware/i2c.h"

typedef enum hour_mode {
	HOUR_MODE_24,
	HOUR_MODE_12
} hour_mode_t;

typedef enum alarm1_mode {
	ALARM1_MATCH_DHMS_DATE   	= 0x00,
	ALARM1_MATCH_DHMS_DAY  		= 0x10,
	ALARM1_MATCH_HMS    		= 0x08,
	ALARM1_MATCH_MS     		= 0x0C,
	ALARM1_MATCH_S      		= 0x0E,
	ALARM1_EVERY_SECOND 		= 0x0F	
} alarm1_mode_t;

typedef enum alarm2_mode {
	ALARM2_MATCH_DHM_DATE    	= 0x00,
	ALARM2_MATCH_DHM_DAY 		= 0x08,
	ALARM2_MATCH_HM          	= 0x0C,
	ALARM2_MATCH_M  			= 0x0E,
	ALARM2_EVERY_MINUTE			= 0x0F	
} alarm2_mode_t;

typedef struct ds3231_inst ds3231_inst_t;

typedef struct ds3231_datetime {
	hour_mode_t hour_mode;
	bool post_meridiem;
	bool century;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
} ds3231_datetime_t;

ds3231_inst_t *ds3231_create(void);
void ds3231_destroy(ds3231_inst_t *inst);

bool ds3231_init(ds3231_inst_t *inst, i2c_inst_t *i2c);

bool ds3231_datetime_get(ds3231_inst_t *inst, ds3231_datetime_t *dt);
bool ds3231_datetime_set(ds3231_inst_t *inst, ds3231_datetime_t *dt);

bool ds3231_alarm1_set(ds3231_inst_t *inst, ds3231_datetime_t *dt, alarm1_mode_t mode);
bool ds3231_alarm2_set(ds3231_inst_t *inst, ds3231_datetime_t *dt, alarm2_mode_t mode);

bool ds3231_alarm_irq_enable(ds3231_inst_t *inst, uint alarm);
bool ds3231_alarm_irq_disable(ds3231_inst_t *inst, uint alarm);

bool ds3231_alarm_clear(ds3231_inst_t *inst, uint alarm);
bool ds3231_alarm_state(ds3231_inst_t *inst, uint alarm);

void ds3231_datetime_print(ds3231_datetime_t *dt);
int ds3231_datetime_compare(ds3231_datetime_t *a, ds3231_datetime_t *b);

#endif //DS3231_PICO_H
