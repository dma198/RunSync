// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define PSAPI_VERSION  1

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers


// Windows Header Files
#include <windows.h>


// C RunTime Header Files
#include <bcrypt.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <objidl.h>
#include <shlwapi.h>
#include <shlobj_core.h>
#include <psapi.h>
#include <Wincrypt.h>
#include <commctrl.h>
#include <windowsx.h>
#include <Uxtheme.h> 
#include <gdiplus.h> /*for gdiplus is required to set global preprocessor definition _HAS_STD_BYTE=0 !*/


#pragma comment (lib,"Shlwapi.lib")
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib,"UxTheme.lib")
#pragma comment (lib,"Psapi.lib")
#pragma comment (lib,"Mpr.lib")
#pragma comment (lib,"Bcrypt.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

