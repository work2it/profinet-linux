/*
 ============================================================================
 Name        : main.c
 Author      : SergDeev
 Version     :
 Copyright   : SergDeev
 Description : Profinet Device
 ============================================================================
 */

#include <stdio.h>
#include "profinet.h"

int main(int argc, char *argv[]) {
	load_config();
    int sock = pn_init();
    if (sock > 0)
	while (1) {
		pn_scan();
		if (is_change) {
			is_change = 0;
			save_config();
		}
	}
}
