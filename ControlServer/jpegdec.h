#ifndef __JPEGDEC_H__
#define __JPEGDEC_H__

int decompress_jpeg_to_raw(unsigned char *inbuffer, unsigned long inlen, unsigned char **outbuffer, unsigned int *width,
			   unsigned int *height, int *pixelsize);

#endif
