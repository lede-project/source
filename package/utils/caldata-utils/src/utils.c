/*
 * 
 * Copyright (C) 2016 Adrian Panella <ianchi74@outlook.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int isMAC(char *s) {
	int i;
    for(i = 0; i < 17; i++) {
        if(i % 3 != 2 && !isxdigit(s[i]))
            return 0;
        if(i % 3 == 2 && s[i] != ':')
            return 0;
    }
    if(s[17] != '\0')
        return 0;
    return 1;
}


int getMTD(char *name)
{
	int device = -1;
	int dev;
	char part[32];
	FILE *fp = fopen("/proc/mtd", "rb");
	if (!fp)
		return -1;
	while (!feof(fp) && fscanf(fp, "%*[^0-9]%d: %*s %*s \"%[^\"]\"", &dev, part) == 2) {
		if (!strcmp(part, name)) {
			device = dev;
			break;
		}
	}
	fclose(fp);
	return device;
}


unsigned short checksum(void *caldata, int size)	
{
	unsigned short *ptr = (unsigned short *)caldata;
	unsigned short crc = 0;
	int i;
	
	for (i = 0; i < size; i += 2) {
		crc ^= *ptr;
		ptr++;
	}
	crc = ~crc;
	return crc;
}


