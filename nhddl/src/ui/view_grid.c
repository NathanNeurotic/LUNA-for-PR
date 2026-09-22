// Original LUNA code: Danny Nunez (dnunezx) 2026
#include "ui/view_internal.h"
#include <stdio.h>

typedef struct {
  float upperLeftX;
  float upperLeftY;
  float upperRightX;
  float upperRightY;
  float lowerLeftX;
  float lowerLeftY;
  float lowerRightX;
  float lowerRightY;
} GridQuad;

static GridQuad gridFlatQuad(int u1, int v1, int u2, int v2, float left, float top, float right, float bottom) {
  GridQuad quad;

  quad.upperLeftX = left + (right - left) * u1 / 1000.0f;
  quad.upperLeftY = top + (bottom - top) * v1 / 1000.0f;
  quad.upperRightX = left + (right - left) * u2 / 1000.0f;
  quad.upperRightY = quad.upperLeftY;
  quad.lowerLeftX = quad.upperLeftX;
  quad.lowerLeftY = top + (bottom - top) * v2 / 1000.0f;
  quad.lowerRightX = quad.upperRightX;
  quad.lowerRightY = quad.lowerLeftY;
  return quad;
}

static void drawGridQuadSolid(GridQuad quad, int z, uint64_t color) {
  gsKit_prim_quad_gouraud(gsGlobal, quad.upperLeftX, quad.upperLeftY, quad.upperRightX, quad.upperRightY, quad.lowerLeftX,
                          quad.lowerLeftY, quad.lowerRightX, quad.lowerRightY, z, color, color, color, color);
}

static void drawGridQuadOutline(GridQuad quad, int z, uint64_t color) {
  gsKit_prim_line(gsGlobal, quad.upperLeftX, quad.upperLeftY, quad.upperRightX, quad.upperRightY, z, color);
  gsKit_prim_line(gsGlobal, quad.upperRightX, quad.upperRightY, quad.lowerRightX, quad.lowerRightY, z, color);
  gsKit_prim_line(gsGlobal, quad.lowerRightX, quad.lowerRightY, quad.lowerLeftX, quad.lowerLeftY, z, color);
  gsKit_prim_line(gsGlobal, quad.lowerLeftX, quad.lowerLeftY, quad.upperLeftX, quad.upperLeftY, z, color);
}

static void drawGridQuadTexture(GSTEXTURE *texture, GridQuad quad, int z, uint64_t color) {
  int previousAlphaTest = gsGlobal->Test->ATST;
  int previousAlphaReference = gsGlobal->Test->AREF;
  int previousAlphaFail = gsGlobal->Test->AFAIL;
  gsKit_TexManager_bind(gsGlobal, texture);
  gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
  gsGlobal->Test->ATST = 2;
  gsGlobal->Test->AREF = 0x80;
  gsGlobal->Test->AFAIL = 0;
  gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
  gsKit_set_test(gsGlobal, GS_ATEST_ON);
  gsKit_prim_quad_texture(gsGlobal, texture, quad.upperLeftX, quad.upperLeftY, 0.0f, 0.0f, quad.upperRightX, quad.upperRightY,
                          texture->Width - 1, 0.0f, quad.lowerLeftX, quad.lowerLeftY, 0.0f, texture->Height - 1,
                          quad.lowerRightX, quad.lowerRightY, texture->Width - 1, texture->Height - 1, z, color);
  gsGlobal->Test->ATST = previousAlphaTest;
  gsGlobal->Test->AREF = previousAlphaReference;
  gsGlobal->Test->AFAIL = previousAlphaFail;
  gsKit_set_test(gsGlobal, GS_ATEST_ON);
  gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
}

static void drawGridTexture(GSTEXTURE *texture, float x, float y, float size, int z, uint64_t color) {
  int previousAlphaTest = gsGlobal->Test->ATST;
  int previousAlphaReference = gsGlobal->Test->AREF;
  int previousAlphaFail = gsGlobal->Test->AFAIL;
  gsKit_TexManager_bind(gsGlobal, texture);
  gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
  gsGlobal->Test->ATST = 2;
  gsGlobal->Test->AREF = 0x80;
  gsGlobal->Test->AFAIL = 0;
  gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
  gsKit_set_test(gsGlobal, GS_ATEST_ON);
  gsKit_prim_sprite_texture(gsGlobal, texture, x, y, 0.0f, 0.0f, x + size, y + size, texture->Width - 1, texture->Height - 1, z, color);
  gsGlobal->Test->ATST = previousAlphaTest;
  gsGlobal->Test->AREF = previousAlphaReference;
  gsGlobal->Test->AFAIL = previousAlphaFail;
  gsKit_set_test(gsGlobal, GS_ATEST_ON);
  gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
}

static void drawGridPage(TargetList *titles, int pageBase, int buffer, int selectedTitleIdx,
                         float left, float top, float right, float bottom, float offsetX, float offsetY,
                         int opacity, int firstRow, int rowCount) {
  if (pageBase < 0 || opacity <= 0)
    return;

  left += offsetX;
  right += offsetX;
  top += offsetY;
  bottom += offsetY;
  int firstSlot = firstRow * GRID_COLUMNS;
  int finalSlot = (firstRow + rowCount) * GRID_COLUMNS;
  if (firstSlot < 0)
    firstSlot = 0;
  if (finalSlot > GRID_PAGE_SIZE)
    finalSlot = GRID_PAGE_SIZE;

  for (int slot = firstSlot; slot < finalSlot; slot++) {
    int row = slot / GRID_COLUMNS;
    int column = slot % GRID_COLUMNS;
    int targetIdx = pageBase + slot;
    int selected = (targetIdx == selectedTitleIdx && targetIdx < titles->total);
    int u1 = column * 250 + 10;
    int v1 = row * 250 + 10;
    int u2 = (column + 1) * 250 - 10;
    int v2 = (row + 1) * 250 - 10;
    GridQuad quad = gridFlatQuad(u1, v1, u2, v2, left, top, right, bottom);
    GridQuad frame = gridFlatQuad(u1 - 4, v1 - 4, u2 + 4, v2 + 4, left, top, right, bottom);

    drawGridQuadSolid(frame, 3, GS_SETREG_RGBA(0x28, 0x68, 0xA8, (0x24 * opacity) / 0x80));
    if (targetIdx >= titles->total) {
      drawGridQuadSolid(quad, 4, GS_SETREG_RGBA(0x02, 0x08, 0x18, (0x38 * opacity) / 0x80));
    } else if (buffer >= 0 && gridCoverLoaded[buffer][slot]) {
      drawGridQuadTexture(gridCoverTextures[buffer][slot], quad, 5,
                          GS_SETREG_RGBA(0x54, 0x64, 0x74, ((selected ? 0x80 : 0x60) * opacity) / 0x80));
    } else {
      float centerX = (quad.upperLeftX + quad.upperRightX + quad.lowerLeftX + quad.lowerRightX) / 4.0f;
      float centerY = (quad.upperLeftY + quad.upperRightY + quad.lowerLeftY + quad.lowerRightY) / 4.0f;
      drawGridQuadSolid(quad, 4, GS_SETREG_RGBA(0x04, 0x14, 0x34, (0x58 * opacity) / 0x80));
      if (opacity >= 0x40) {
        snprintf(lineBuffer, sizeof(lineBuffer), "%d", targetIdx + 1);
        drawTextWindow(centerX - 18, centerY - getFontLineHeight() / 2, centerX + 18,
                       centerY + getFontLineHeight() / 2, 5, HeaderTextColor, ALIGN_CENTER, lineBuffer);
      }
    }
  }
}

static void drawGridHighlight(int selectedTitleIdx, int windowBase, float left, float top, float right, float bottom,
                              float offsetX, int opacity) {
  if (selectedTitleIdx >= windowBase && selectedTitleIdx < windowBase + GRID_PAGE_SIZE) {
    int selectedSlot = selectedTitleIdx - windowBase;
    int highlightU = (selectedSlot % GRID_COLUMNS) * 250 + 10;
    int highlightV = (selectedSlot / GRID_COLUMNS) * 250 + 10;
    left += offsetX;
    right += offsetX;
    GridQuad highlight = gridFlatQuad(highlightU - 10, highlightV - 10, highlightU + 240,
                                      highlightV + 240, left, top, right, bottom);
    drawGridQuadSolid(highlight, 6, GS_SETREG_RGBA(0x24, 0x78, 0xD8, (0x30 * opacity) / 0x80));
    drawGridQuadOutline(highlight, 7, GS_SETREG_RGBA(0xC8, 0xF0, 0xFF, (0x78 * opacity) / 0x80));
  }
}

static void drawGridSelectionPlate(int selectedTitleIdx, int windowBase, float left, float top, float right, float bottom,
                                   float offsetX, int opacity) {
  if (selectedTitleIdx < windowBase || selectedTitleIdx >= windowBase + GRID_PAGE_SIZE)
    return;

  int selectedSlot = selectedTitleIdx - windowBase;
  int plateU = (selectedSlot % GRID_COLUMNS) * 250;
  int plateV = (selectedSlot / GRID_COLUMNS) * 250;
  left += offsetX;
  right += offsetX;

  // Sit just outside the cover quad so the dark-blue plate remains visible
  // as a stable halo behind the selected artwork.
  GridQuad plate = gridFlatQuad(plateU + 2, plateV + 2, plateU + 248, plateV + 248, left, top, right, bottom);
  drawGridQuadSolid(plate, 4, GS_SETREG_RGBA(0x0C, 0x38, 0x78, (0x38 * opacity) / 0x80));
}

void drawPSBBNGrid(TargetList *titles, int selectedTitleIdx, int activeWindowBase, int activeWindowBuffer,
                   int incomingWindowBase, int incomingWindowBuffer, int selectedCoverBuffer,
                   int cascadeDirection, int cascadeProgress, uint32_t frameNowMs) {
  const int top = headerHeight + 12;
  const int bottom = gsGlobal->Height - footerHeight - 10;
  const int leftX = keepoutArea + 10;
  const int leftRight = gsGlobal->Width * 49 / 100;
  const float gridLeft = leftX + 7;
  const float gridTop = top + 29;
  const float gridRight = leftRight - 5;
  const float gridBottom = bottom - 18;
  const int rightLeft = leftRight + 24;
  const int rightRight = gsGlobal->Width - keepoutArea;
  const int selectedAreaTop = top + getFontLineHeight() + 8;
  const int selectedAreaBottom = bottom - getFontLineHeight() - 8;
  int selectedSize = rightRight - rightLeft;
  int selectedX;
  int selectedY;
  int displayPageBase = activeWindowBase;
  int pageCount = (titles->total + GRID_PAGE_SIZE - 1) / GRID_PAGE_SIZE;
  char selectedTitle[255];

  if (selectedSize > selectedAreaBottom - selectedAreaTop)
    selectedSize = selectedAreaBottom - selectedAreaTop;
  selectedX = rightLeft + (rightRight - rightLeft - selectedSize) / 2;
  selectedY = selectedAreaTop + (selectedAreaBottom - selectedAreaTop - selectedSize) / 2;

  drawSharedLibraryBackground(frameNowMs);
  drawTextWindow(leftX, headerHeight - getFontLineHeight(), leftRight, 0, 5, HeaderTextColor, ALIGN_LEFT, "GRID");

  if (cascadeProgress > 0 && incomingWindowBase >= 0) {
    float travel = (gridRight - gridLeft) / GRID_COLUMNS + 8.0f;
    displayPageBase = incomingWindowBase;

    for (int row = 0; row < GRID_ROWS; row++) {
      int outgoingProgress = lunaNavEase(lunaNavGridCascadeProgress(cascadeProgress, row, 0));
      int incomingProgress = lunaNavEase(lunaNavGridCascadeProgress(cascadeProgress, row, 1));
      float outgoingOffset = ((cascadeDirection > 0) ? -travel : travel) * outgoingProgress / 1000.0f;
      float incomingOffset = ((cascadeDirection > 0) ? travel : -travel) * (1000 - incomingProgress) / 1000.0f;
      int outgoingOpacity = (0x80 * (1000 - outgoingProgress)) / 1000;
      int incomingOpacity = (0x80 * incomingProgress) / 1000;

      drawGridSelectionPlate(selectedTitleIdx, activeWindowBase, gridLeft, gridTop, gridRight, gridBottom,
                             outgoingOffset, outgoingOpacity);
      drawGridSelectionPlate(selectedTitleIdx, incomingWindowBase, gridLeft, gridTop, gridRight, gridBottom,
                             incomingOffset, incomingOpacity);
      drawGridPage(titles, activeWindowBase, activeWindowBuffer, selectedTitleIdx, gridLeft, gridTop, gridRight, gridBottom,
                   outgoingOffset, 0.0f, outgoingOpacity, row, 1);
      drawGridPage(titles, incomingWindowBase, incomingWindowBuffer, selectedTitleIdx, gridLeft, gridTop, gridRight, gridBottom,
                   incomingOffset, 0.0f, incomingOpacity, row, 1);

      if (selectedTitleIdx >= incomingWindowBase + row * GRID_COLUMNS &&
          selectedTitleIdx < incomingWindowBase + (row + 1) * GRID_COLUMNS)
        drawGridHighlight(selectedTitleIdx, incomingWindowBase, gridLeft, gridTop, gridRight, gridBottom,
                          incomingOffset, incomingOpacity);
    }
  } else {
    drawGridSelectionPlate(selectedTitleIdx, activeWindowBase, gridLeft, gridTop, gridRight, gridBottom, 0.0f, 0x80);
    drawGridPage(titles, activeWindowBase, activeWindowBuffer, selectedTitleIdx, gridLeft, gridTop, gridRight, gridBottom,
                 0.0f, 0.0f, 0x80, 0, GRID_ROWS);
    drawGridHighlight(selectedTitleIdx, activeWindowBase, gridLeft, gridTop, gridRight, gridBottom, 0.0f, 0x80);
  }

  snprintf(lineBuffer, sizeof(lineBuffer), "Page %d/%d", displayPageBase / GRID_PAGE_SIZE + 1, pageCount);
  drawTextWindow(leftX, headerHeight - getFontLineHeight(), leftRight, 0, 5, HeaderTextColor, ALIGN_RIGHT, lineBuffer);

  formatPSBBNTitle(getTargetByIdx(titles, selectedTitleIdx)->name, selectedTitle, rightRight - rightLeft);
  drawTextWindow(rightLeft, top - 2, rightRight, 0, 6, FontMainColor, ALIGN_HCENTER, selectedTitle);
  if (selectedCoverBuffer >= 0 && gridSelectedLoaded[selectedCoverBuffer]) {
    gsKit_prim_sprite(gsGlobal, selectedX - 3, selectedY - 3, selectedX + selectedSize + 3, selectedY + selectedSize + 3, 3,
                      GS_SETREG_RGBA(0x28, 0x88, 0xD8, 0x28));
    drawGridTexture(gridSelectedTextures[selectedCoverBuffer], selectedX, selectedY, selectedSize, 5,
                    GS_SETREG_RGBA(0x80, 0x80, 0x80, 0x80));
  } else {
    gsKit_prim_sprite(gsGlobal, selectedX, selectedY, selectedX + selectedSize, selectedY + selectedSize, 4,
                      GS_SETREG_RGBA(0x04, 0x14, 0x34, 0x58));
    drawTextWindow(selectedX, selectedY, selectedX + selectedSize, selectedY + selectedSize, 5, HeaderTextColor, ALIGN_CENTER,
                   (selectedCoverBuffer < 0) ? "FAST TRACK" : "ART\nUNAVAILABLE");
  }
  snprintf(lineBuffer, sizeof(lineBuffer), "%d/%d", selectedTitleIdx + 1, titles->total);
  drawTextWindow(rightLeft, selectedY + selectedSize + 4, rightRight, 0, 6, FontMainColor, ALIGN_RIGHT, lineBuffer);

  int baseY = gsGlobal->Height - footerHeight + 8;
  int circleX = 22;
  int crossX = 154;
  int triangleX = 286;
  drawIconWindow(circleX, baseY, 0, gsGlobal->Height, 6, FontMainColor, ALIGN_CENTER, ICON_CIRCLE);
  drawTextWindow(circleX + getIconWidth(ICON_CIRCLE) + 6, baseY, crossX - 8, gsGlobal->Height, 6, FontMainColor, ALIGN_VCENTER, "Views");
  drawIconWindow(crossX, baseY, 0, gsGlobal->Height, 6, FontMainColor, ALIGN_CENTER, ICON_CROSS);
  drawTextWindow(crossX + getIconWidth(ICON_CROSS) + 6, baseY, triangleX - 8, gsGlobal->Height, 6, FontMainColor, ALIGN_VCENTER, "Launch");
  drawIconWindow(triangleX, baseY, 0, gsGlobal->Height, 6, FontMainColor, ALIGN_CENTER, ICON_TRIANGLE);
  drawTextWindow(triangleX + getIconWidth(ICON_TRIANGLE) + 6, baseY, gsGlobal->Width - keepoutArea, gsGlobal->Height, 6, FontMainColor, ALIGN_VCENTER,
                  "Options");
}
