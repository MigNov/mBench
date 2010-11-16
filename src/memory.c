/*
 * memory.c: Memory-related functions
 *
 * Author: Michal Novotny <minovotn@redhat.com>
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include "utils.h"

float memory_size_get(int type)
{
	float memsize, val = 0.0;
	long pageSize, pages;

	pageSize = sysconf(_SC_PAGESIZE);
	pages = sysconf(_SC_PHYS_PAGES);
	memsize = pageSize * pages;

	switch (type) {
		case MEMTYPE_B:
					val = memsize;
					break;
		case MEMTYPE_KB:
					val = (memsize / 1024);
					break;
		case MEMTYPE_MB:
					val = (memsize / 1048576);
					break;
		case MEMTYPE_GB:
					val = (memsize / 1073741824);
					break;
		default:
					return -1;
	}

	return val;
}

void memory_display(char *address, int length) {
	int i = 0;
	char *line = (char*)address;
	unsigned char ch;
	printf("%08X | ", (int)address);
	while (length-- > 0) {
		printf("%02X ", (unsigned char)*address++);
		if (!(++i % 16) || (length == 0 && i % 16)) {
			if (length == 0) { while (i++ % 16) { printf("__ "); } }
			printf("| ");
			while (line < address) {
				ch = *line++;
				printf("%c", (ch < 33 || ch == 255) ? 0x2E : ch);
			}
			if (length > 0) { printf("\n%08X | ", (int)address); }
		}
	}
	puts("");
}

