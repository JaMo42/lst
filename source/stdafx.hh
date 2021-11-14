#ifndef STDAFX_HH
#define STDAFX_HH

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <ctime>
#include <cstring>

#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <list>
#include <bitset>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <chrono>

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#define NOMINMAX
#include <Windows.h>

#include <io.h>
#define isatty _isatty
#define fileno _fileno

#include <ShObjIdl_core.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <comdef.h>

#include <AccCtrl.h>
#include <AclAPI.h>

#else // _WIN32

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif // _WIN32

#include "arena_alloc.hh"

#define def auto
#define let auto

namespace fs = std::filesystem;
using namespace std::literals;

#endif // STDAFX_HH