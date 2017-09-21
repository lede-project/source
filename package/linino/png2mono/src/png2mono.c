#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <png.h>

int main(int argc, char **argv)
{
	unsigned char hdr[8];
	int ret = 0;
	char errstr[512];
	int x, y;
	int w, h, rowsize;
	unsigned char b;
	unsigned char thres = 128;

	png_structp ppng;
	png_infop ppng_info;
	png_bytep *rows;
	FILE *fin = NULL, *fout = NULL;

	errstr[0] = '\0';

	if (argc < 3) {
		printf("\npng2mono: Converts png to monochrome raw images"
			"\n\tUsage: <fname_png> <fname_mono> [threshold]"
			"\n\tthreshold is optional and defaults to %u\n\n",
			thres);
		return 0;
	}
        
	/* Open input and output file */
	fin = fopen(argv[1], "r");
	if (!fin) {
		ret = 1;
		snprintf(errstr, sizeof(errstr) - 1,
				"Failed to open input file %s\n", argv[1]);
		goto done;
	}

	fout = fopen(argv[2], "w+");
	if (!fout) {
		ret = 2;
		snprintf(errstr, sizeof(errstr) - 1,
				"Failed to open output file %s\n", argv[2]);
		goto done;
	}

	if (argc >= 4)
		thres = atoi(argv[3]);

	fread(hdr, 1, 8, fin);

	if (png_sig_cmp(hdr, 0, 8)) {
		ret = 3;
		sprintf(errstr, "File %s is not a PNG image\n", argv[1]);
		goto done;
	}

	ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
				      NULL);
	ppng_info = png_create_info_struct(ppng);

	png_init_io(ppng, fin);
	png_set_sig_bytes(ppng, 8);

	png_read_info(ppng, ppng_info);

	if (ppng->color_type != PNG_COLOR_TYPE_GRAY) {
		ret = 4;
		sprintf(errstr, "Input image must be a grayscale image "
			"with no alpha channel\n");
		goto done;
	}

	w = png_get_image_width(ppng, ppng_info);
	h = png_get_image_height(ppng, ppng_info);
	
	rows = (png_bytep*)malloc(sizeof(png_bytep) * h);

	rowsize = w * 4;

	if (png_get_bit_depth(ppng, ppng_info) == 16)
		rowsize *= 2;

	for (y = 0; y < h; y++)
		rows[y] = (png_byte*)malloc(rowsize);

	png_read_image(ppng, rows);

	png_set_expand(ppng);
	png_set_strip_16(ppng);

	for (y = 0; y < h; y++) {
		png_byte *r = rows[y];
		for (x = 0; x < w; x++) {
			png_byte *pix = &(r[x]);

			if (*pix >= thres)
				b |= 0x80;

			if (x % 8 == 7) {
				int i;
				fprintf(fout, "%c", b);
				for (i = 0; i < 8; i++)
					printf("%s", b & (1 << i) ? "*" : ".");
				b = 0;
			}
			else
				b >>= 1;
		}
		printf("\n");
	}

done:
	printf("%s", errstr);
	if (fin)
		fclose(fin);
	if (fout)
		fclose(fout);
	return ret;
}
