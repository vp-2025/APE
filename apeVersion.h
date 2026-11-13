#pragma once

// the version of ape++: for ape++.rc2 (VERSIONINFO) and the About dialog
#define APE_VER_MAJOR 1
#define APE_VER_MINOR 4
#define APE_VER_PATCH 0
#define APE_VER_BUILD 0

#define APE_PRODUCT   "APE++"
#define APE_COMPANY   "vp"
#define APE_COPYRIGHT "Copyright (C) 2009-2026"

#define APE_STR_(x) #x
#define APE_STR(x)  APE_STR_(x)
#define APE_VERSION_STR APE_STR(APE_VER_MAJOR) "." APE_STR(APE_VER_MINOR) "." APE_STR(APE_VER_PATCH)
