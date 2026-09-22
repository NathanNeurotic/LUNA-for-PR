// Original LUNA code: Danny Nunez (dnunezx) 2026
#include "ui/view_internal.h"

#include <stdio.h>

typedef struct {
  int x;
  int y;
  int size;
} ConstellationPoint;

static const uint16_t constellationX[PSBBN_COVER_CACHE_COUNT] = {95, 230, 360, 680, 450, 265, 115, 835, 915, 495};
static const uint16_t constellationY[PSBBN_COVER_CACHE_COUNT] = {210, 85, 415, 455, 880, 760, 610, 125, 570, 155};
static const uint8_t constellationSize[PSBBN_COVER_CACHE_COUNT] = {38, 46, 52, 166, 52, 45, 38, 46, 40, 42};
static const uint8_t constellationConnections[][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
    {1, 9}, {9, 7}, {7, 3}, {7, 8}, {8, 4}, {2, 9},
};

static ConstellationPoint constellationAnchor(int anchor, uint32_t now) {
  const int top = headerHeight + 10;
  const int bottom = gsGlobal->Height - footerHeight - 12;
  uint32_t phase = (uint32_t)(((uint64_t)now << 16) / 9000ULL) + anchor * 4915U;
  ConstellationPoint point;

  point.x = (gsGlobal->Width * constellationX[anchor]) / 1000 + (discWave(phase) * 3) / 127;
  point.y = top + ((bottom - top) * constellationY[anchor]) / 1000 + (discWave(phase + (8 << 11)) * 2) / 127;
  point.size = constellationSize[anchor];
  return point;
}

static ConstellationPoint constellationPointAt(int slotPosition, uint32_t now, int *visibility) {
  int lower;
  int fraction;
  ConstellationPoint first;
  ConstellationPoint second;
  ConstellationPoint result;

  *visibility = 1000;
  if (slotPosition < 0) {
    *visibility = 1000 + slotPosition;
    if (*visibility < 0)
      *visibility = 0;
    slotPosition = 0;
  } else if (slotPosition > (PSBBN_COVER_CACHE_COUNT - 1) * 1000) {
    *visibility = 1000 - (slotPosition - (PSBBN_COVER_CACHE_COUNT - 1) * 1000);
    if (*visibility < 0)
      *visibility = 0;
    slotPosition = (PSBBN_COVER_CACHE_COUNT - 1) * 1000;
  }

  lower = slotPosition / 1000;
  fraction = slotPosition % 1000;
  if (lower >= PSBBN_COVER_CACHE_COUNT - 1) {
    lower = PSBBN_COVER_CACHE_COUNT - 1;
    fraction = 0;
  }
  first = constellationAnchor(lower, now);
  second = constellationAnchor((lower < PSBBN_COVER_CACHE_COUNT - 1) ? lower + 1 : lower, now);
  fraction = lunaNavEase(fraction);
  result.x = first.x + ((second.x - first.x) * fraction) / 1000;
  result.y = first.y + ((second.y - first.y) * fraction) / 1000;
  result.size = first.size + ((second.size - first.size) * fraction) / 1000;
  return result;
}

static void drawConstellationBrackets(int centerX, int centerY, int size, int z, uint64_t color) {
  int half = size / 2 + 5;
  int arm = (size > 100) ? 16 : 7;
  int left = centerX - half;
  int right = centerX + half;
  int top = centerY - half;
  int bottom = centerY + half;

  gsKit_prim_line(gsGlobal, left, top, left + arm, top, z, color);
  gsKit_prim_line(gsGlobal, left, top, left, top + arm, z, color);
  gsKit_prim_line(gsGlobal, right - arm, top, right, top, z, color);
  gsKit_prim_line(gsGlobal, right, top, right, top + arm, z, color);
  gsKit_prim_line(gsGlobal, left, bottom, left + arm, bottom, z, color);
  gsKit_prim_line(gsGlobal, left, bottom - arm, left, bottom, z, color);
  gsKit_prim_line(gsGlobal, right - arm, bottom, right, bottom, z, color);
  gsKit_prim_line(gsGlobal, right, bottom - arm, right, bottom, z, color);
}

// Constellation is a spatial map rather than another row or matrix. Ten nearby
// games occupy a connected star chart; navigation moves their artwork between
// nodes while the game nearest the lock point resolves at full size.
void drawConstellation(TargetList *titles, int selectedTitleIdx, GSTEXTURE **covers, int flowOffset,
                       int randomActive, uint32_t frameNowMs) {
  uint32_t now = frameNowMs;
  ConstellationPoint anchors[PSBBN_COVER_CACHE_COUNT];
  int position[PSBBN_COVER_CACHE_COUNT];
  int visualFocus = -1;
  int visualFocusDistance = 0x7FFFFFFF;
  char selectedTitle[255];

  drawSharedLibraryBackground(frameNowMs);
  drawTextWindow(keepoutArea + 10, headerHeight - getFontLineHeight(), gsGlobal->Width - keepoutArea, 0, 6,
                 HeaderTextColor, ALIGN_LEFT, "CONSTELLATION");
  if (randomActive)
    drawTextWindow(keepoutArea + 10, headerHeight - getFontLineHeight(), gsGlobal->Width - keepoutArea, 0, 6,
                   FontMainColor, ALIGN_RIGHT, "RANDOM SCAN");

  for (int anchor = 0; anchor < PSBBN_COVER_CACHE_COUNT; anchor++)
    anchors[anchor] = constellationAnchor(anchor, now);

  // Quiet range marks make the large lock point read as an instrument rather
  // than simply a bigger cover, while leaving the artwork edge unobstructed.
  {
    ConstellationPoint focus = anchors[PSBBN_COVER_CACHE_FOCUS];
    int half = focus.size / 2;
    gsKit_prim_line(gsGlobal, focus.x - half - 22, focus.y, focus.x - half - 9, focus.y, 3,
                    GS_SETREG_RGBA(0x88, 0xD8, 0xF8, 0x38));
    gsKit_prim_line(gsGlobal, focus.x + half + 9, focus.y, focus.x + half + 22, focus.y, 3,
                    GS_SETREG_RGBA(0x88, 0xD8, 0xF8, 0x38));
    gsKit_prim_line(gsGlobal, focus.x, focus.y - half - 18, focus.x, focus.y - half - 8, 3,
                    GS_SETREG_RGBA(0x88, 0xD8, 0xF8, 0x38));
    gsKit_prim_line(gsGlobal, focus.x, focus.y + half + 8, focus.x, focus.y + half + 18, 3,
                    GS_SETREG_RGBA(0x88, 0xD8, 0xF8, 0x38));
  }

  for (unsigned int edge = 0; edge < sizeof(constellationConnections) / sizeof(constellationConnections[0]); edge++) {
    int from = constellationConnections[edge][0];
    int to = constellationConnections[edge][1];
    gsKit_prim_line(gsGlobal, anchors[from].x, anchors[from].y, anchors[to].x, anchors[to].y, 2,
                    GS_SETREG_RGBA(0x38, 0x88, 0xB8, 0x26));
  }

  // One moving pulse gives the chart a readable direction without making all
  // of the connecting lines flash at once.
  {
    unsigned int connectionCount = sizeof(constellationConnections) / sizeof(constellationConnections[0]);
    unsigned int pulseEdge = (now / 650U) % connectionCount;
    int pulseProgress = (int)(((now % 650U) * 1000U) / 650U);
    int from = constellationConnections[pulseEdge][0];
    int to = constellationConnections[pulseEdge][1];
    int pulseX = anchors[from].x + ((anchors[to].x - anchors[from].x) * pulseProgress) / 1000;
    int pulseY = anchors[from].y + ((anchors[to].y - anchors[from].y) * pulseProgress) / 1000;
    gsKit_prim_sprite(gsGlobal, pulseX - 4, pulseY - 1, pulseX + 4, pulseY + 1, 3,
                      GS_SETREG_RGBA(0xA8, 0xE8, 0xFF, 0x68));
    gsKit_prim_sprite(gsGlobal, pulseX - 1, pulseY - 4, pulseX + 1, pulseY + 4, 3,
                      GS_SETREG_RGBA(0xA8, 0xE8, 0xFF, 0x68));
  }

  for (int cacheIdx = 0; cacheIdx < PSBBN_COVER_CACHE_COUNT; cacheIdx++) {
    int distance;
    position[cacheIdx] = (cacheIdx - PSBBN_COVER_CACHE_FOCUS) * 1000 + flowOffset;
    distance = (position[cacheIdx] < 0) ? -position[cacheIdx] : position[cacheIdx];
    if (distance < visualFocusDistance) {
      visualFocusDistance = distance;
      visualFocus = cacheIdx;
    }
  }

  // Draw the focal node last so its full-resolution art and lock brackets stay
  // visually dominant when another node crosses behind it.
  for (int pass = 0; pass < 2; pass++) {
    for (int cacheIdx = 0; cacheIdx < PSBBN_COVER_CACHE_COUNT; cacheIdx++) {
      int isFocus = (cacheIdx == visualFocus);
      int visibility;
      int slotPosition;
      int distance;
      int emphasis;
      int targetIdx;
      int x1;
      int y1;
      ConstellationPoint point;

      if ((pass == 0 && isFocus) || (pass == 1 && !isFocus))
        continue;
      slotPosition = position[cacheIdx] + PSBBN_COVER_CACHE_FOCUS * 1000;
      point = constellationPointAt(slotPosition, now, &visibility);
      if (visibility <= 0)
        continue;
      distance = (position[cacheIdx] < 0) ? -position[cacheIdx] : position[cacheIdx];
      emphasis = 1000 - ((distance > 1000) ? 1000 : distance);
      targetIdx = lunaNavWrap(titles->total, selectedTitleIdx + cacheIdx - PSBBN_COVER_CACHE_FOCUS);
      x1 = point.x - point.size / 2;
      y1 = point.y - point.size / 2;
      gsKit_prim_sprite(gsGlobal, x1 - 5, y1 - 5, x1 + point.size + 5, y1 + point.size + 5, 3,
                        GS_SETREG_RGBA(0x28, 0x98, 0xD8, ((isFocus ? 0x38 : 0x18) * visibility) / 1000));
      if (covers[cacheIdx] != NULL && psbbnCoverLoaded[cacheIdx]) {
        drawPSBBNCover(covers[cacheIdx], x1, y1, point.size, cacheIdx, emphasis, visibility);
      } else {
        gsKit_prim_sprite(gsGlobal, x1, y1, x1 + point.size, y1 + point.size, 4,
                          GS_SETREG_RGBA(0x04, 0x14, 0x34, (0x58 * visibility) / 1000));
        if (isFocus)
          drawTextWindow(x1, y1, x1 + point.size, y1 + point.size, 6,
                         HeaderTextColor, ALIGN_CENTER, "ART\nUNAVAILABLE");
      }

      drawConstellationBrackets(point.x, point.y, point.size, 7,
                                GS_SETREG_RGBA(isFocus ? 0xD0 : 0x68, isFocus ? 0xF4 : 0xC8, 0xFF,
                                               ((isFocus ? 0x70 : 0x30) * visibility) / 1000));
      if (!isFocus) {
        snprintf(lineBuffer, sizeof(lineBuffer), "%d", targetIdx + 1);
        drawTextWindow(x1, y1 + point.size + 2, x1 + point.size,
                       y1 + point.size + getFontLineHeight() + 2, 7,
                       HeaderTextColor, ALIGN_HCENTER, lineBuffer);
      }
    }
  }

  if (visualFocus >= 0) {
    int focusTargetIdx = lunaNavWrap(titles->total, selectedTitleIdx + visualFocus - PSBBN_COVER_CACHE_FOCUS);
    int titleLeft = gsGlobal->Width * 52 / 100;
    int titleRight = gsGlobal->Width - keepoutArea;
    int titleY = anchors[PSBBN_COVER_CACHE_FOCUS].y + anchors[PSBBN_COVER_CACHE_FOCUS].size / 2 + 11;
    formatPSBBNTitle(getTargetByIdx(titles, focusTargetIdx)->name, selectedTitle, titleRight - titleLeft);
    gsKit_prim_sprite(gsGlobal, titleLeft - 5, titleY - 3, titleRight + 2,
                      titleY + getFontLineHeight() * 2 + 3, 3, GS_SETREG_RGBA(0x04, 0x18, 0x38, 0x42));
    gsKit_prim_line(gsGlobal, titleLeft - 5, titleY - 3, titleRight + 2, titleY - 3, 4,
                    GS_SETREG_RGBA(0x58, 0xB8, 0xE8, 0x34));
    drawTextWindow(titleLeft, titleY, titleRight, 0, 8, FontMainColor,
                   ALIGN_HCENTER, selectedTitle);
    snprintf(lineBuffer, sizeof(lineBuffer), "%d/%d", focusTargetIdx + 1, titles->total);
    drawTextWindow(titleLeft, titleY + getFontLineHeight(), titleRight, 0, 8, HeaderTextColor,
                   ALIGN_HCENTER, lineBuffer);
  }

  int baseY = gsGlobal->Height - footerHeight + 8;
  int circleX = 26;
  int squareX = gsGlobal->Width * 27 / 100;
  int crossX = gsGlobal->Width * 52 / 100;
  int triangleX = gsGlobal->Width * 76 / 100;
  drawIconWindow(circleX, baseY, 0, gsGlobal->Height, 8, FontMainColor, ALIGN_CENTER, ICON_CIRCLE);
  drawTextWindow(circleX + getIconWidth(ICON_CIRCLE) + 6, baseY, squareX - 8, gsGlobal->Height, 8, FontMainColor,
                 ALIGN_VCENTER, "Orbit");
  drawIconWindow(squareX, baseY, 0, gsGlobal->Height, 8, FontMainColor, ALIGN_CENTER, ICON_SQUARE);
  drawTextWindow(squareX + getIconWidth(ICON_SQUARE) + 6, baseY, crossX - 8, gsGlobal->Height, 8, FontMainColor,
                 ALIGN_VCENTER, "Random");
  drawIconWindow(crossX, baseY, 0, gsGlobal->Height, 8, FontMainColor, ALIGN_CENTER, ICON_CROSS);
  drawTextWindow(crossX + getIconWidth(ICON_CROSS) + 6, baseY, triangleX - 8, gsGlobal->Height, 8, FontMainColor,
                 ALIGN_VCENTER, "Launch");
  drawIconWindow(triangleX, baseY, 0, gsGlobal->Height, 8, FontMainColor, ALIGN_CENTER, ICON_TRIANGLE);
  drawTextWindow(triangleX + getIconWidth(ICON_TRIANGLE) + 6, baseY, gsGlobal->Width - keepoutArea,
                 gsGlobal->Height, 8, FontMainColor, ALIGN_VCENTER, "Options");
}
