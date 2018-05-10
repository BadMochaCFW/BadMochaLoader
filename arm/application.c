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
#include <stdlib.h> //is malloc broken?
#include <string.h>
#include <stdbool.h>

#include "system/smc.h"
#include "system/ppc_elf.h"
#include "system/memory.h"
#include "system/irq.h"
#include "system/smc.h"
#include "system/latte.h"
#include "storage/isfs.h"
#include "common/utils.h"
#include "storage/sd/sdcard.h"
#include "storage/sd/fatfs/elm.h"
#include "common/ini.h"

#include "application.h"
#include "ipc_protocol.h"

static const char* kernel_locs[] = {
	"sdmc:/linux/dtbImage.wiiu",
	"sdmc:/linux/kernel"
};

struct LoaderConfig {
	char defaultProfile[64];
	char nextBootProfile[64];
};
static struct LoaderConfig ldrConfig = { 0 };

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
	//Uncomment to completely erase linux-loader from MEM2
	//memset32((void*)0x10000000, 0, 0x800000);

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
		}

		//writeback ctrl value to reset IPC
		write32(LT_IPC_ARMCTRL_COMPAT, ctrl);
	}
}

static size_t search_for_profile(const char* name) {
/*	Check if the profile exists */
	for (size_t i = 0; i < NUM_PROFILES; i++) {
		if (profiles[i].enabled) {
			if (strcmp(name, profiles[i].name) == 0) {
				return i;
			}
		}
	}
/*	If not, find the first unused slot and initialise it */
	for (size_t i = 0; i < NUM_PROFILES; i++) {
		if (!profiles[i].enabled) {
			strncpy(profiles[i].name, name, sizeof(profiles[i].name));
			profiles[i].enabled = true;
			return i;
		}
	}
	return ~0;
}

static int config_handler(void* user, const char* section, const char* name, const char* value) {
	if (strcmp("loader", section) == 0) {
		if (strcmp("nextBoot", name) == 0) {
			strncpy(ldrConfig.defaultProfile, value, sizeof(ldrConfig.nextBootProfile));
		} else if (strcmp("default", name) == 0) {
			strncpy(ldrConfig.defaultProfile, value, sizeof(ldrConfig.defaultProfile));
		}
	} else if (strncmp("profile:", section, 8) == 0) {
		size_t ndx = search_for_profile(section + 8);
		if (ndx == ~0) {
			printf("Couldn't allocate profile for %s\n", section);
			return 0;
		}
		if (strcmp("name", name) == 0) {
			strncpy(profiles[ndx].humanName, value, sizeof(profiles[ndx].humanName));
		} else if (strcmp("kernel", name) == 0) {
			strncpy(profiles[ndx].kernelPath, value, sizeof(profiles[ndx].kernelPath));
		} else if (strcmp("cmdline", name) == 0) {
			strncpy(profiles[ndx].kernelCmd, value, sizeof(profiles[ndx].kernelCmd));
		}
	}
	return 1;
}

char * read_line(FILE *fin) { //thx StackExchange. TODO: audit, clean up
    char *buffer;
    char *tmp;
    int read_chars = 0;
    int bufsize = 512;
    char *line = malloc(bufsize);

    if (!line) return NULL;

    buffer = line;

    while ( fgets(buffer, bufsize - read_chars, fin) ) {
        read_chars = strlen(line);

        if ( line[read_chars - 1] == '\n' ) {
            line[read_chars - 1] = '\0';
            return line;
        }

        else {
            bufsize = 2 * bufsize;
            tmp = realloc(line, bufsize);
            if ( tmp ) {
                line = tmp;
                buffer = line + read_chars;
            }
            else {
                free(line);
                return NULL;
            }
        }
    }
    return NULL;
}

void NORETURN app_run() {
	int res;
	bool kernel_loaded = false;

/*	Clear out the PowerPC comms area */
	memset(ppc_data, 0, sizeof(*ppc_data));

/*	It doesn't really matter if this fails */
	ini_parse("sdmc:/linux/boot.cfg", &config_handler, NULL);

/*	Attempt to find nextBoot profile */
	size_t profileNdx;
	bool profileFound = false;
	for (profileNdx = 0; profileNdx < NUM_PROFILES; profileNdx++) {
		if (!profiles[profileNdx].enabled) continue;
		if (strcmp(ldrConfig.nextBootProfile, profiles[profileNdx].name) == 0) {
			printf("[BOOT] Next-boot config found, skipping default config\n");
			profileFound = true;
			/*	Since nextBoot was found successfully, comment it out so it doesn't boot next time */
				FILE* config = fopen("sdmc:/linux/boot.cfg", "r");
				FILE* newconfig_temp = fopen("sdmc:/linux/newConfigTemp.cfg", "w");
				char *line=NULL;
				while ( (line = read_line(config)) != NULL ) {
					if (strncmp(line, "nextBoot", 8)!=0){
						printf("[BOOT] Found line to comment out: %s\n", line);
						fputc(';', newconfig_temp);
					}
					fprintf(newconfig_temp, "%s\n", line);
					free(line);
				}
				fclose(config);
				fclose(newconfig_temp);
				remove("sdmc:/linux/boot.cfg");
				rename("sdmc:/linux/newConfigTemp.cfg", "sdmc:/linux/boot.cfg");
			break;
		}
	}
	
/*	If that failed, find the user-selected default profile */
	if(!profileFound) {
		printf("[BOOT] No next-boot config found, using default config\n");
		for (profileNdx = 0; profileNdx < NUM_PROFILES; profileNdx++) {
			if (!profiles[profileNdx].enabled) continue;
			if (strcmp(ldrConfig.defaultProfile, profiles[profileNdx].name) == 0) {
				profileFound = true;
				break;
			}
		}
	}

/*	Load kernel according to config file profile */
	if (profileFound) {
		printf("[INFO] Trying to load kernel from %s...\n", profiles[profileNdx].kernelPath);
		res = ppc_load_file(profiles[profileNdx].kernelPath, &ppc_entry);
		if (res >= 0) kernel_loaded = true;

	/*	Put kernel commandline at end of memory, ready for the boot wrapper to read */
		if (strlen(profiles[profileNdx].kernelCmd) > 0) {
			strncpy(ppc_data->cmdline, profiles[profileNdx].kernelCmd, sizeof(ppc_data->cmdline));
			write32((unsigned int)&ppc_data->magic, WIIU_LOADER_MAGIC);
		}
	}

/*	If that failed, use the default kernel locations */
	if (!kernel_loaded) {
	/*	Try each deafault location */
		for (int i = 0; i < sizeof(kernel_locs) / sizeof(const char*); i++) {
			printf("[INFO] Trying to load kernel from %s...\n", kernel_locs[i]);
			res = ppc_load_file(kernel_locs[i], &ppc_entry);
			if (res >= 0) {
				kernel_loaded = true;
				break;
			}
		}
	}

	if (!kernel_loaded) {
		printf("[FATL] Loading PowerPC kernel failed! (%d)\n", res);
		panic(0);
	}
	printf("[ OK ] Loaded PowerPC kernel (%d). Entry is %08lX.\n", res, ppc_entry);

	//Shut everything down, ready for SRAM switch
	ELM_Unmount();
	sdcard_exit();
	irq_disable(IRQ_SD0);
	isfs_fini();
	irq_shutdown();
	printf("[ OK ] Unmounted filesystems and removed interrupts.\n");
	mem_shutdown();
	printf("[ OK ] Disabled caches/MMU\n");

	printf("[BYE ] Doing SRAM context switch...\n");

	//Move execution to SRAM. Linux will overwrite everything in MEM2, including this code.
	sram_ctx_switch(&app_run_sram);
}
