#ifndef __JPEGENC_H__
#define __JPEGENC_H__

int compress_rgb_to_jpeg(char *image, int x, int y, int width, int height, int stretch, int bpp,
			 unsigned char **outbuffer, long unsigned int *outlen, int quality);

#endif
