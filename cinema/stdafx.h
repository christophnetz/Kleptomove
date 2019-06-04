// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//  are changed infrequently
//

#pragma once

// Change these values to use different versions
#ifndef WINVER
#define WINVER		0x0A00
#define _WIN32_WINNT	0x0A00
#define _WIN32_IE	0x0A00
#define _RICHEDIT_VER	0x0A00
#endif

#include <Unknwnbase.h>
#include <atldef.h>
#include <atlexcept.h>
#include <atlbase.h>
#include <wtl/atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#include <wtl/atlframe.h>
#include <wtl/atlctrls.h>
#include <wtl/atldlgs.h>
#include <wtl/atlctrlw.h>
#include <shellapi.h> 
#pragma comment (lib, "shell32.lib") 
#include <wtl/atlctrlx.h>
#include <wtl/atlsplit.h>
#include <wtl/atlmisc.h>



#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
