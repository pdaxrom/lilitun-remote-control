#ifndef __FBOPS_H__
#define __FBOPS_H__

void fbops_bitblit(uint8_t * dst, int dst_w, int dst_h, int dst_depth, uint8_t * src, int src_x, int src_y, int src_w,
		   int src_h, int src_depth, int pixfmt);

#endif
