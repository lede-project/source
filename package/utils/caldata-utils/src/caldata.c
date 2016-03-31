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
 * caldata
 * 
 * Utility to manipulate calibration data for ath10k drivers.
 * It enables to extract from MTD partition or file and to patch MAC address fixing the checksum.
 * 
 */
 

#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

#include "caldata.h"


/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'i':
      arguments->input_file = arg;
      break;

    case 'f':
      arguments->output_file = arg;
      break;

    case 'o':
	  sscanf(arg,"%li", &arguments->offset);
      break;

    case 's':
	  sscanf(arg,"%li", &arguments->size);
      break;

    case 'a':
      
      if(!isMAC(arg))
		argp_error(state,"address is not a valid MAC address");

      arguments->macaddr = arg;
      break;

    case 'v':
      arguments->verify = 1;
      break;

	case ARGP_KEY_END: //all options processed, verify consistency

		if(!arguments->input_file) 
			argp_error(state,"No input file or partition specified.");
			
		if(!arguments->verify && !arguments->output_file) 
			argp_error(state, "Must specify either -f FILE or -v.");
			
		break;
	
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


int loadfile(char *file, void **data, int offset, int size) {
	*data=NULL;	
	char *source = NULL;
	
	
	char mtdpath[32];
	FILE *fp=NULL;
	
	//try input as partition
	int mtdnr = getMTD(file);
	if(mtdnr>=0) {
		sprintf(mtdpath, "/dev/mtdblock%d", mtdnr);
		fp = fopen(mtdpath, "rb");
	}
	//try as file
	if(!fp) 
		fp = fopen(file, "rb");
	
	if (fp != NULL) {
		/* Go to the end of the file. */
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			long filesize = ftell(fp);
			if (filesize == -1)  {
				fclose(fp);
				return -1; }

			size= size ? size : (filesize - offset);
			if(size + offset > filesize) {
				fputs("Offset + size gets past EOF", stderr);
				fclose(fp);
				exit(1);}
			
			source = malloc(size + 1);

			/* offset from the start of the file. */
			if (fseek(fp, offset, SEEK_SET) != 0) { 
				free(source);
				fclose(fp);
				return -1;}

			/* Read file into memory. */
			size_t newLen = fread(source, sizeof(char), size, fp);
			if (newLen == 0) {
				free(source);
				fclose(fp);
				return -1;
			} 
			else {
				fclose(fp);
				*data=source;
				return size;
			}
		}
	}
	return -1;
}

int main (int argc, char **argv)
{
	struct arguments arguments;
	unsigned short ret=0;

	/* Default values. */
	arguments.input_file = NULL;
	arguments.output_file = NULL;
	arguments.offset = 0;
	arguments.size = 0;
	arguments.macaddr = NULL;
	arguments.verify = 0;

	/* Parse arguments and validate*/
	argp_parse (&argp, argc, argv, 0, 0, &arguments);
	
	
	// read input data
	unsigned short  *data;
	int data_len=loadfile(arguments.input_file,(void **)&data, arguments.offset, arguments.size);
	
	if(data_len<0) {
		fputs("Error reading input file/partition.\n", stderr);
		exit(1);
		}
	
	unsigned char *mac=(unsigned char *)data+6;
	
	if(arguments.verify) {

		printf("Size: %d\n", data[0]);
		printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		ret=checksum(data, data[0]) ? 1 : 0;
		printf("Checksum: 0x%04x (%s)\n", data[1], ret ? "ERROR" : "OK");
		if(ret) 
			fputs("Calibration data checksum error\n", stderr);

		free(data);
		exit(ret);
	}
	
	if(arguments.macaddr) {
		sscanf(arguments.macaddr,"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		data[1]=0;
		data[1]=checksum(data, data[0]);
	}

	FILE *fp=fopen(arguments.output_file,"wb");
	
	if(!fp) {
		fputs("Error opening output file\n", stderr);
		free(data);
		exit(1);
	}
	
	if(fwrite(data, data_len, 1, fp)!=1) {
		fputs("Error writing output file\n", stderr);
		free(data);
		fclose(fp);
		exit(1);
	}
	

	fclose(fp);
	free(data);
	exit(0);
}
