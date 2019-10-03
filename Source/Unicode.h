#pragma once
#include "stdafx.h"


#if defined(UNICODE) || defined(_UNICODE)
#pragam message("You should be using UTF-8.")

using tstring = std::wstring;
#define tstrcmp wcscmp
#define tcout std::wcout

#else

using tstring = std::string;
#define tstrcmp strcmp
#define tcout std::cout

#endif