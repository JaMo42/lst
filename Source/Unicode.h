#pragma once
#include "stdafx.h"


#if defined(UNICODE) || defined(_UNICODE)
#pragma message("You should be using UTF-8.")

using tstring = std::wstring;
#define tstrcmp wcscmp
#define tcout std::wcout
#define tsnprint wsprintfW
#define tprintf wprintf
#define tstrftime wcsftime
#define to_tstring std::to_wstring

#else

using tstring = std::string;
#define tstrcmp strcmp
#define tcout std::cout
#define tsnprintf snprintf
#define tprintf printf
#define tstrftime strftime
#define to_tstring std::to_string

#endif