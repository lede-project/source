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
 
/*
 * 
 * scksum
 * 
 * Utility for calculating a simple checksum (mainly for for ath10k calibration data).
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
 
unsigned short checksum(FILE *fp)	
{
	unsigned short data;
	unsigned short crc = 0;
	int i;

	while (fread(&data, sizeof(unsigned short), 1, fp) == 1) { 
		
		crc ^= data;
	}

	if(ferror(fp)) {
		fprintf(stderr, "Error reading input.\n");
		exit(1);
	}
	
	crc = ~crc;

	return crc;
}

void usage(char *prog)
{
	fprintf(stderr, "Usage: %s [-b | -h] [file]\n\n", prog);
	fprintf(stderr, "\t-b: output in binary format.\n\t-h: output in hex format (default).\n");
	fprintf(stderr, "\t[file] to process, blank or - to use stdin.\n");
}


int main(int argc, char **argv) {
	FILE *input;
	int bflag = 0;
	int hflag = 0;
	int c;
	unsigned short crc = 0;
	opterr = 0;

	while((c = getopt(argc, argv, "hb")) != -1)
	switch(c)
	  {
	  case 'b':
		bflag = 1;
		break;
		
	  case 'h':
		hflag = 1;
		break;
		
	  case '?':
		if (isprint(optopt))
		  fprintf(stderr, "Unknown option '-%c'.\n", optopt);
		else
		  fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
		usage(argv[0]);
		return 1;
		
	  default:
		usage(argv[0]);
		return 1;
	  }

	if( bflag && hflag ) {
		fprintf(stderr, "Only one output format can be selected [-h | -b]\n");
		usage(argv[0]);
		return 1;
	}
		
	if( (argc - optind) > 1 ) {
		fprintf(stderr, "Provide only one input file.\n");
		usage(argv[0]);
		return 1;
	}
	
	if( optind == argc || argv[optind] == "-" )
		input = stdin;
	else {
		input = fopen(argv[optind],"rb");
		if( NULL == input) {
			fprintf(stderr, "Unable to open '%s'\n", argv[optind]);
			return 1;
		}
	}

	crc=checksum(input);
	
	if(bflag)
		fwrite(&crc, sizeof(unsigned short), 1, stdout);
		
	else
		printf("%x\n",crc);
		
	return 0;
}

