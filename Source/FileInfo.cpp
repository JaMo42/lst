#include "stdafx.h"
#include "FileInfo.h"

FileInfo::FileInfo(const WIN32_FIND_DATA &FindData)
	: Name(FindData.cFileName),
	Creation(FindData.ftCreationTime),
	LastWrite(FindData.ftLastWriteTime),
	Size(FindData.nFileSizeLow),
	Type(FILETYPE::Normal),
	LinkName{},
	LinkOK{false},
	Hidden(FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN || FindData.cFileName[0] == _T('.'))
{
	// Get file/directory handle
	Handle = CreateFile(
		Name.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		// Set flag for getting handle of either a directory or a file
		(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	// Try to resolve filetype from flags
	for (unsigned i = 0; i < (unsigned)FILETYPE::COUNT; i++)
		if (FiletypeAttributes[i] & FindData.dwFileAttributes) {
			Type = (FILETYPE)i;
			break;
		}
	// If not possible, try to from extension
	if (Type == FILETYPE::Normal) {
		const size_t LastDot = Name.find_last_of(_T('.'));
		if (LastDot != std::string::npos) {
			// Check for executable of shortcut
			const tstring Extension = Name.substr(LastDot + 1);
			if (Extension == _T("exe")) {
				Type = FILETYPE::Executable;
			} else if (Extension == _T("lnk")) {
				Type = FILETYPE::Shortcut;
			}
		// If the file has no extension it is makred as unknown
		} else {
			Type = FILETYPE::Unknown;
		}
	}
	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
		// Resolve the link target
		TCHAR buf[MAX_PATH];
		LinkOK = GetFinalPathNameByHandle(Handle, buf, MAX_PATH, NULL);
		if (LinkOK) {
			LinkName = buf + 4;
		}
	}
}
