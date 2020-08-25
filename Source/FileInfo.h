#pragma once
#include "Unicode.h"
#include "FileType.h"

/**
 * Holds file information.
 */
struct FileInfo {
  // The handle of the file
	HANDLE Handle;
  // The file type
	FILETYPE Type;
  // The name of the file
	tstring Name;
  // For symlinks, the name of the linked file
	tstring LinkName;
  // For symlinks, whether to linked file exists
	bool LinkOK;
  // The creation time
	FILETIME Creation;
  // The time of the last modification
	FILETIME LastWrite;
  // The size of the file
	DWORD Size;
  // Whether the file is hidden
	bool Hidden;

  /**
   * @param FindData - WIN32_FIND_DATA structure to get file information from.
   */
	FileInfo(const WIN32_FIND_DATA &FindData);
};
