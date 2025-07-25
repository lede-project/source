#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int file_size(char *filename)
{
	struct stat statbuf;

	stat(filename, &statbuf);
	int size = statbuf.st_size;

	return size;
}

int main(int argc, char **argv)
{
	char *fileName_in = argv[1];
	char fileName_out[256];
	FILE *f_in = NULL;
	FILE *f_out = NULL;
	int i = 0, size = 0;
	unsigned char byte;

	size = file_size(fileName_in);
	if (fileName_in == NULL || size <= 0) {
		printf("\nInvalid input file!\nUsage:\n ./bin2hex fw_name\n\n");
		return -1;
	}
	memset(fileName_out, 0, sizeof(fileName_out));
	snprintf(fileName_out, sizeof(fileName_out), "%s.hex", fileName_in);

	f_in = fopen(fileName_in, "rb");
	if (f_in == NULL)
		return -1;
	f_out = fopen(fileName_out, "wt");
	if (f_out == NULL) {
		fclose(f_in);
		return -1;
	}

	for (i = 0; i < size; i++) {
		byte = fgetc(f_in);
		if (i != 0 && i % 16 == 0)
			fprintf(f_out, "\n");
		fprintf(f_out, "0x%02X,", 0xff&byte);
	}
	fclose(f_in);
	fclose(f_out);

	printf("\ndone!\noutput file: %s\n\n", fileName_out);

	return 0;
}

