#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "fbops.h"

void fbops_bitblit(uint8_t * dst, int dst_w, int dst_h, int dst_depth, uint8_t * src, int src_x, int src_y, int src_w,
		   int src_h, int src_depth, int pixfmt)
{
//fprintf(stderr, "blit %d, %d, %d -- %d, %d, %d, %d, %d\n",
//                  dst_w, dst_h, dst_depth, src_x, src_y, src_w, src_h, src_depth);
    char bytespp_dst = dst_depth / 8;
    char bytespp_src = src_depth / 8;
    uint8_t *ptr_d = dst + dst_w * src_y * bytespp_dst + src_x * bytespp_dst;
    int y = src_y;
    int w = ((dst_w - src_x) < src_w) ? (dst_w - src_x) : src_w;
    while (src_h && y < dst_h) {
	for (int i = 0; i < w; i++) {
	    if ((bytespp_dst == 3 || bytespp_dst == 4) && (bytespp_src == 3 || bytespp_src == 4)) {
		ptr_d[i * bytespp_dst + 0] = src[i * bytespp_src + 0];
		ptr_d[i * bytespp_dst + 1] = src[i * bytespp_src + 1];
		ptr_d[i * bytespp_dst + 2] = src[i * bytespp_src + 2];
		if (bytespp_dst == 4) {
		    if (bytespp_src == 3) {
			ptr_d[i * bytespp_dst + 3] = 0;
		    } else {
			ptr_d[i * bytespp_dst + 3] = src[i * bytespp_src + 3];
		    }
		}
	    }
	}
	ptr_d += dst_w * bytespp_dst;
	src += src_w * bytespp_src;
	src_h--;
	y++;
    }
}
