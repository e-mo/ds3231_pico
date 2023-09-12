/* DS3231 drivers for the RPi Pico. 
 *
 * Maintainer: Evan Morse
 * 			   tech@emorse.dev
 */
#ifndef DS3231_PICO_H
#define DS3231_PICO_H

#define DS3231_I2C_ADDRESS 0x68

typedef enum hour_mode {
	HOUR_MODE_24,
	HOUR_MODE_12
} hour_mode_t;

typedef enum alarm1_mode {
	ALARM_MATCH_DHMS_DATE   = 0x00,
	ALARM_MATCH_DHMS_DAY  	= 0x10,
	ALARM_MATCH_HMS    		= 0x08,
	ALARM_MATCH_MS     		= 0x0C,
	ALARM_MATCH_S      		= 0x0E,
	ALARM_EVERY_SECOND 		= 0x0F,	
} alarm1_mode_t;

typedef enum alarm2_mode {
	ALARM_EVERY_MINUTE,	
	ALARM_MATCH_M,
	ALARM_MATCH_HM,
	ALARM_MATCH_HM,
	ALARM_MATCH_DHM
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

bool ds3231_alarm1_set(ds3231_inst_t inst, ds3231_datetime_t dt, alarm1_mode_t mode);
bool ds3231_alarm2_set(ds3231_inst_t inst, ds3231_datetime_t dt, alarm2_mode_t mode);

bool ds3231_alarm_enable(ds3231_inst_t inst, uint alarm);
bool ds3231_alarm_disable(ds3231_inst_t inst, uint alarm);

#endif //DS3231_PICO_H
