#ifndef LUNA_UI_VIEW_STATE_H
#define LUNA_UI_VIEW_STATE_H

#include "target.h"
#include "ui/navigation.h"

// The selected view is stored with the selected title's metadata on its game drive.
// Missing or invalid state falls back to Classic.
UILibraryView loadLastLibraryView(Target *target);
int saveLastLibraryView(Target *target, UILibraryView view);

#endif
