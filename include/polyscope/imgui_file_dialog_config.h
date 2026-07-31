#pragma once

#include "ImGuiFileDialogConfig.h"
#include "IconFontCppHeaders/IconsLucide.h"

#define USE_PLACES_FEATURE
#define PLACES_PANE_DEFAULT_SHOWN false
#define USE_PLACES_BOOKMARKS
#define USE_PLACES_DEVICES

#define dirEntryString ICON_LC_FOLDER " "
#define linkEntryString ICON_LC_FILE_SYMLINK " "
#define fileEntryString ICON_LC_FILE " "

#define createDirButtonString ICON_LC_FOLDER_PLUS
#define resetButtonString ICON_LC_REFRESH_CW
#define editPathButtonString ICON_LC_PENCIL

#define addPlaceButtonString ICON_LC_BOOKMARK_PLUS
#define removePlaceButtonString ICON_LC_BOOKMARK_MINUS
#define validatePlaceButtonString ICON_LC_CHECK
#define editPlaceButtonString ICON_LC_PENCIL
