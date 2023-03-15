/*
 *	It's a Project! linux-loader
 *
 *	Copyright (C) 2017          Ash Logan <quarktheawesome@gmail.com>
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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "video/gfx.h"
#include "system/smc.h"
#include "system/ppc_elf.h"
#include "system/memory.h"
#include "system/irq.h"
#include "system/smc.h"
#include "system/latte.h"
#include "storage/nand/nand.h"
#include "storage/nand/isfs/isfs.h"
#include "common/utils.h"
#include "storage/sd/sdcard.h"
#include "storage/sd/fatfs/elm.h"
#include "common/ini.h"

#include "application.h"
#include "ipc_protocol.h"




#define NUM_PROFILES 4
struct ProfileConfig {
	bool enabled;
	char name[64];
	char humanName[64];
	char kernelPath[128];
	char kernelCmd[256];
};
static struct ProfileConfig profiles[NUM_PROFILES] = { false };

/* If this struct ever has to be changed in a non-ABI compatible way,
   change the magic.
   Past magics:
    - 0xCAFEFECA: initial version
*/
#define WIIU_LOADER_MAGIC 0xCAFEFECA
struct wiiu_ppc_data {
	unsigned int magic;
	char cmdline[256];
	void* initrd;
	unsigned int initrd_sz;
};
static struct wiiu_ppc_data* ppc_data = (void*)0x89200000;

#define LT_IPC_ARMCTRL_COMPAT_X1 0x4
#define LT_IPC_ARMCTRL_COMPAT_Y1 0x1

u32 SRAM_DATA ppc_entry = 0;

static void SRAM_TEXT NORETURN app_run_sram() {
	//Uncomment to completely erase -loader from MEM2
	//memset32((void*)0x10000000, 0, 0x800000);
	gfx_clear(GFX_DRC, BLACK);
	//Start the PowerPC
	write32(0x14000000, ppc_entry);
	for (;;) {
		//check for message sent flag
		u32 ctrl = read32(LT_IPC_ARMCTRL_COMPAT);
		if (!(ctrl & LT_IPC_ARMCTRL_COMPAT_X1)) continue;

		//read PowerPC's message
		u32 msg = read32(LT_IPC_PPCMSG_COMPAT);

		//process commands
		if (msg == CMD_POWEROFF) {
			smc_shutdown(false);
		} else if (msg == CMD_REBOOT) {
			smc_shutdown(true);
		} else if ((msg & CMD_MASK) == CMD_PRINT) {
			char buf[4];
			buf[0] = (msg & 0x00FF0000) >> 16;
			buf[1] = (msg & 0x0000FF00) >>  8;
			buf[2] = (msg & 0x000000FF) >>  0;
			buf[3] = '\0';
			sram_print(buf);
		}

		//writeback ctrl value to reset IPC
		write32(LT_IPC_ARMCTRL_COMPAT, ctrl);
	}
}




void NORETURN app_run() {
      app_run_sram()
      printf("Hello From App Kernel");

}
