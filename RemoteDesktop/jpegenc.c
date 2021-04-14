#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jpeglib.h>
#include "jpegenc.h"

int compress_rgb_to_jpeg(char *image, int x, int y, int width, int height, int stretch, int bpp,
			 unsigned char **outbuffer, long unsigned int *outlen, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    int z = y;

    *outbuffer = NULL;
    *outlen = 0;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, outbuffer, outlen);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 4;
    cinfo.in_color_space = JCS_EXT_BGRA;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    unsigned char scanline[width * bpp / 8];

    while (cinfo.next_scanline < cinfo.image_height) {
	memcpy(scanline, &image[z * stretch + x * (bpp / 8)], width * (bpp / 8));
	row_pointer[0] = scanline;
	jpeg_write_scanlines(&cinfo, row_pointer, 1);
	z++;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return (0);
}
