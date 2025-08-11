#include "video.h"
#include "alloc.h"
#include "debug.h"
#include "limine.h"
#include "string.h"
#include "utils.h"

vidbuf default_video_buf;

int video_init() {
  if (framebuffer_request.response->framebuffers == NULL) {
    return 0;
  }
  struct limine_framebuffer *f = framebuffer_request.response->framebuffers[0];
  vidbuf *d = &default_video_buf;
// d->info.width = f->width;
#define _V(a) d->info.a = f->a
  _V(width);
  _V(height);
  _V(pitch);
  _V(bpp);
  _V(memory_model);
  _V(red_mask_size);
  _V(red_mask_shift);
  _V(green_mask_size);
  _V(green_mask_shift);
  _V(blue_mask_size);
  _V(blue_mask_shift);
#undef _V(a)
  default_video_buf.front = f->address;
  default_video_buf.back = (void *)malloc(f->height * f->width);
  if (default_video_buf.back == NULL) {
    panic("falied alloc default_video_buf.back\n");
  }

  plogk("video has been inited. buf addr:\n\t%p(front)\n\t%p(back)\n",
        default_video_buf.front, default_video_buf.back);
  return 1;
}

int video_flush(vidbuf *buf) {
  plogk("flush video. info:\n\tfront:%p\n\tback:%p\n\th:%ld\tw:%ld\n", buf->front,
        buf->back, buf->info.height, buf->info.width);
  return memcpy(buf->front, buf->back, buf->info.height * buf->info.width * 4);
}
