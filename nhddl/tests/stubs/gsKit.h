#ifndef TEST_GSKIT_H
#define TEST_GSKIT_H
#include <stdint.h>
typedef uint32_t u32;
typedef struct { int unused; } GSGLOBAL;
typedef struct {
  u32 *Mem, *Clut;
  int Width, Height, Vram, VramClut, PSM, Filter, Delayed;
} GSTEXTURE;
#define GS_FILTER_LINEAR 1
#define GS_PSM_CT32 0
#define GS_MODE_NTSC 2
#define GS_MODE_PAL 3
#define GS_MODE_DTV_480P 4
#define GS_SETREG_RGBA(r, g, b, a) ((uint64_t)(r) | ((uint64_t)(g) << 8) | ((uint64_t)(b) << 16) | ((uint64_t)(a) << 24))
void gsKit_TexManager_bind(GSGLOBAL *gs, GSTEXTURE *texture);
void gsKit_TexManager_free(GSGLOBAL *gs, GSTEXTURE *texture);
void gsKit_TexManager_invalidate(GSGLOBAL *gs, GSTEXTURE *texture);
#endif
