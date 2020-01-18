/*
 *	It's a Project! linux-loader
 *
 *	Copyright (C) 2017          Ash Logan <quarktheawesome@gmail.com>
 *	Copyright (C) 2020          Roberto Van Eeden <rwrr0644@gmail.com>
 *
 *	Based on code from the following contributors:
 *
 *	Copyright (C) 2016          SALT
 *	Copyright (C) 2016          Daz Jones <daz@dazzozo.com>
 *
 *	Copyright (C) 2008, 2009    Haxx Enterprises <bushing@gmail.com>
 *	Copyright (C) 2008, 2009    Sven Peter <svenpeter@gmail.com>
 *	Copyright (C) 2008, 2009    Hector Martin "marcan" <marcan@marcansoft.com>
 *	Copyright (C) 2009          Andre Heider "dhewg" <dhewg@wiibrew.org>
 *	Copyright (C) 2009          John Kelley <wiidev@kelley.ca>
 *
 *	(see https://github.com/Dazzozo/minute)
 *
 *	This code is licensed to you under the terms of the GNU GPL, version 2;
 *	see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 */

#ifdef LOLSERIAL_OUTPUT

#include "lolserial.h"
#include "system/gpio.h"
#include "common/utils.h"
#include "system/latte.h"

/* the number of timer ticks to wait
 * each bit; might need manual adjustment
 * for particoular serial adapters, usually
 * the stable value is in the +5/-5 range */
#define TICKS_PER_BIT	(200)


static ALWAYS_INLINE void set_sensorbar(int s) {
	if (s) {
		clear32(LT_GPIO_OUT, GP_SENSORBAR);
	} else {
		set32(LT_GPIO_OUT, GP_SENSORBAR);
	}
}

static void SRAM_TEXT timer_wait(u32 ticks)
{
	u32 now = read32(LT_TIMER);
	u32 then = now + ticks;
	if(then < now) {
		while(read32(LT_TIMER) >= now);
		now = read32(LT_TIMER);
	}
	while(now < then) {
		now = read32(LT_TIMER);
	}
}

void SRAM_TEXT lolserial_print(char *str) {
	for (int i = 0; str[i]; i++) {
		char c = str[i];
		set_sensorbar(0);
		timer_wait(TICKS_PER_BIT);
		for (int j = 0; j < 8; j++) {
			set_sensorbar(c & 1);
			timer_wait(TICKS_PER_BIT);
			c >>= 1;
		}
		set_sensorbar(1);
		timer_wait(TICKS_PER_BIT);
	}
}

void lolserial_init() {
	clear32(LT_GPIO_OWNER, GP_SENSORBAR);
	set32(LT_GPIO_ENABLE, GP_SENSORBAR);
	set32(LT_GPIO_DIR, GP_SENSORBAR);
	set_sensorbar(1);
}

#endif /* LOLSERIAL_OUTPUT */
