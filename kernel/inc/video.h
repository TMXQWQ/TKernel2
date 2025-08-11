#pragma once

#include "stdint.h"

typedef struct Video_Info{
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
}vidinfo;

typedef struct Video_Buf
{
    void *front, *back; //前后缓冲
    vidinfo info;
} vidbuf;

extern vidbuf default_video_buf;

int video_init();
int video_flush(vidbuf*);
