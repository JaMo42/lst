#pragma once
#include "Unicode.h"
#include "Filetype.h"


struct FileInfo {
	FILETYPE Type;
	tstring Name;
	tstring LinkName;
	bool LinkOK;
	FILETIME Creation;
	FILETIME LastWrite;
	ULONG64 Size;
	bool Hidden;

	FileInfo(const WIN32_FIND_DATA &FindData);
};
