// Exercise the production cache with a counted PNG decoder and fake GS uploads.
#include "devices/devices.h"
#include "ui/art_cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GSGLOBAL gs;
GSGLOBAL *gsGlobal = &gs;
static int decodes, missingID = -1, fullResident, peakFullResident;

void gsKit_TexManager_bind(GSGLOBAL *unused, GSTEXTURE *t) {
  (void)unused;
  if (!t->Vram && t->Width > 64) {
    fullResident++;
    if (fullResident > peakFullResident) peakFullResident = fullResident;
  }
  t->Vram = 1;
}
void gsKit_TexManager_free(GSGLOBAL *unused, GSTEXTURE *t) {
  (void)unused;
  if (t->Vram && t->Width > 64) fullResident--;
  t->Vram = 0;
}
void gsKit_TexManager_invalidate(GSGLOBAL *unused, GSTEXTURE *t) {
  gsKit_TexManager_free(unused, t);
}
int decodePNGTextureRGBA(GSGLOBAL *unused, GSTEXTURE *t, const char *path) {
  (void)unused;
  int id = atoi(strrchr(path, '/') + 1);
  decodes++;
  if (id == missingID) return -1;
  t->Width = t->Height = 128;
  t->Mem = malloc(128 * 128 * sizeof(*t->Mem));
  assert(t->Mem);
  for (int i = 0; i < 128 * 128; i++) t->Mem[i] = (u32)(id + 1);
  return 0;
}
int loadPNGTextureRGBA(GSGLOBAL *g, GSTEXTURE *t, const char *path) {
  int result = decodePNGTextureRGBA(g, t, path);
  if (!result) gsKit_TexManager_bind(g, t);
  return result;
}
int gsKit_texture_png(GSGLOBAL *g, GSTEXTURE *t, const char *path) {
  return loadPNGTextureRGBA(g, t, path);
}
Target *getTargetByIdx(TargetList *list, int index) {
  for (Target *t = list->first; t; t = t->next)
    if (t->idx == index) return t;
  assert(!"invalid target index");
  return NULL;
}

static void verifyCovers(TargetList *list, int focus, int collection) {
  int expected[PSBBN_COVER_CACHE_COUNT];
  lunaCollectionCacheLayout(list->total, focus, 0, expected);
  for (int i = 0; i < PSBBN_COVER_CACHE_COUNT; i++) {
    int index = collection ? expected[i] : lunaNavWrap(list->total, focus + i - PSBBN_COVER_CACHE_FOCUS);
    if (index < 0) { assert(!psbbnCoverLoaded[i]); continue; }
    int id = atoi(getTargetByIdx(list, index)->id);
    if (id == missingID) { assert(!psbbnCoverLoaded[i]); continue; }
    assert(psbbnCoverLoaded[i]);
    GSTEXTURE *t = psbbnCoverTextures[i];
    assert((t->Mem[t->Width * (t->Height / 2) + t->Width / 2] & 0xffffff) == (u32)(id + 1));
  }
}

int main(void) {
  struct DeviceMapEntry device = {.mountpoint = "test:"};
  Target entries[20] = {0};
  char ids[20][8];
  for (int i = 0; i < 20; i++) {
    snprintf(ids[i], sizeof(ids[i]), "%d", i);
    entries[i].idx = i;
    entries[i].id = ids[i];
    entries[i].device = &device;
    entries[i].next = i < 19 ? &entries[i+1] : NULL;
  }
  TargetList list = {.total = 20, .first = entries, .last = &entries[19]};
  assert(artCacheInit() == 0);
  prepareCollectionCovers(&list, 5);
  assert(decodes == 10);
  verifyCovers(&list, 5, 1);
  u32 *focusPixels = psbbnCoverTextures[PSBBN_COVER_CACHE_FOCUS]->Mem;
  u32 *tailPixels = psbbnCoverTextures[9]->Mem;
  // A full view round-trip at the same title needs no new PNG decode or
  // thumbnail allocation, including when another view evicted all GS memory.
  for (int repeat = 0; repeat < 10; repeat++) {
    releasePSBBNCoverVRAM();
    assert(fullResident == 0);
    prepareCollectionCovers(&list, 5);
    assert(decodes == 10);
    assert(psbbnCoverTextures[PSBBN_COVER_CACHE_FOCUS]->Mem == focusPixels);
    assert(psbbnCoverTextures[9]->Mem == tailPixels);
    verifyCovers(&list, 5, 1);
  }
  assert(peakFullResident <= 2);
  // Orbit rotations keep Collection's identity metadata aligned.
  refreshPSBBNCovers(&list, 5, 5);
  assert(decodes == 10);
  refreshPSBBNCovers(&list, 6, 5);
  verifyCovers(&list, 6, 0);
  updatePSBBNCoverResidency(0);
  refreshPSBBNCovers(&list, 5, 6);
  verifyCovers(&list, 5, 0);
  updatePSBBNCoverResidency(0);
  int before = decodes;
  prepareCollectionCovers(&list, 5);
  assert(decodes == before);
  verifyCovers(&list, 5, 1);
  // Rapid scanning remaps without decoding; entry then fills only missing art.
  refreshCollectionCovers(&list, 14, 0, 1, 0, 1);
  assert(decodes == before);
  prepareCollectionCovers(&list, 14);
  verifyCovers(&list, 14, 1);
  // Missing art is remembered across view changes rather than retried forever.
  releasePSBBNCovers();
  missingID = 5;
  prepareCollectionCovers(&list, 5);
  before = decodes;
  releasePSBBNCoverVRAM();
  prepareCollectionCovers(&list, 5);
  assert(decodes == before);
  verifyCovers(&list, 5, 1);
  // A filtered list with the same ranks has different title identities.
  Target filtered[2] = {entries[12], entries[17]};
  filtered[0].idx = 0; filtered[0].next = &filtered[1];
  filtered[1].idx = 1; filtered[1].next = NULL;
  TargetList favorites = {.total = 2, .first = filtered, .last = &filtered[1]};
  prepareCollectionCovers(&favorites, 0);
  assert(decodes == before + 2);
  verifyCovers(&favorites, 0, 1);
  refreshPSBBNCovers(&favorites, 0, 0);
  verifyCovers(&favorites, 0, 0);
  prepareCollectionCovers(&favorites, 0);
  verifyCovers(&favorites, 0, 1);
  assert(peakFullResident <= 2);
  releasePSBBNCovers();
  artCacheShutdown();
  puts("art cache tests passed");
  return 0;
}
