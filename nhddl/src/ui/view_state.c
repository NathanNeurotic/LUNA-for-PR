#include "ui/view_state.h"
#include "devices/devices.h"
#include "dprintf.h"
#include "options.h"
#include <errno.h>
#include <ps2sdkapi.h>
#include <stdio.h>
#include <string.h>

static const char lastViewPath[] = "/lastView.txt";
static const char lastViewTempPath[] = "/lastView.txt.tmp";
static const char *const viewNames[] = {
    "classic", "collection", "grid", "orbit"};

static struct DeviceMapEntry *viewDevice(Target *target) {
  if (target == NULL || target->device == NULL)
    return NULL;
  return target->device->metadev ? target->device->metadev : target->device;
}

static int readViewFile(const char *path, UILibraryView *view) {
  char name[24];
  FILE *file = fopen(path, "r");
  if (file == NULL)
    return -ENOENT;
  if (fgets(name, sizeof(name), file) == NULL) {
    fclose(file);
    return -EINVAL;
  }
  fclose(file);
  size_t nameLength = strcspn(name, "\r\n");
  if (name[nameLength] == '\0')
    return -EINVAL;
  name[nameLength] = '\0';
  for (int i = UI_VIEW_CLASSIC; i <= UI_VIEW_ORBIT; i++) {
    if (!strcmp(name, viewNames[i])) {
      *view = (UILibraryView)i;
      return 0;
    }
  }
  return -EINVAL;
}

UILibraryView loadLastLibraryView(Target *target) {
  struct DeviceMapEntry *device = viewDevice(target);
  char path[PATH_MAX];
  UILibraryView view;

  if (device == NULL || device->mountpoint == NULL)
    return UI_VIEW_CLASSIC;
  // A completed temporary file is the latest state if power was lost between
  // removing the previous file and committing the replacement.
  if (!buildConfigFilePath(path, sizeof(path), device->mountpoint, lastViewTempPath) &&
      !readViewFile(path, &view)) {
    DPRINTF("Restored library view %s from %s\n", viewNames[view], path);
    return view;
  }
  if (!buildConfigFilePath(path, sizeof(path), device->mountpoint, lastViewPath) &&
      !readViewFile(path, &view)) {
    DPRINTF("Restored library view %s from %s\n", viewNames[view], path);
    return view;
  }
  return UI_VIEW_CLASSIC;
}

int saveLastLibraryView(Target *target, UILibraryView view) {
  struct DeviceMapEntry *device = viewDevice(target);
  char directory[PATH_MAX];
  char path[PATH_MAX];
  char tempPath[PATH_MAX];
  struct stat st;
  FILE *file;

  if (device == NULL || device->mountpoint == NULL ||
      view < UI_VIEW_CLASSIC || view > UI_VIEW_ORBIT)
    return -EINVAL;
  if (buildConfigFilePath(directory, sizeof(directory), device->mountpoint, NULL) ||
      buildConfigFilePath(path, sizeof(path), device->mountpoint, lastViewPath) ||
      buildConfigFilePath(tempPath, sizeof(tempPath), device->mountpoint, lastViewTempPath))
    return -ENAMETOOLONG;
  if (stat(directory, &st) == -1 && mkdir(directory, 0777)) {
    DPRINTF("ERROR: Failed to create view state directory: %d\n", errno);
    return -EIO;
  }
  file = fopen(tempPath, "w");
  if (file == NULL)
    return -EIO;
  int writeResult = fprintf(file, "%s\n", viewNames[view]);
  int closeResult = fclose(file);
  if (writeResult < 0 || closeResult) {
    remove(tempPath);
    return -EIO;
  }
  remove(path);
  if (rename(tempPath, path)) {
    DPRINTF("ERROR: Failed to commit last view: %d\n", errno);
    return -EIO;
  }
  DPRINTF("Saved library view %s to %s\n", viewNames[view], path);
  return 0;
}
