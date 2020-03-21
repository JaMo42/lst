#pragma once

// File types
enum class FILETYPE {
	Normal,
	Unknown,
	Directory,
	Executable,
	Link,
	Shortcut,
	System,
	Virtual,
	Device,
	COUNT
};

// Display letters and indicators for each filetype
constexpr const TCHAR FiletypeChars[] = _T("-?dxlsDv");

// Windows file attributes for file types.
// If the value is 0, the respective filetype is determined by something else.
constexpr DWORD FiletypeAttributes[] = {
	0,  // No flag for this
	0,  // No flag for this
	FILE_ATTRIBUTE_DIRECTORY,
	0,  // Determined by extension
	FILE_ATTRIBUTE_REPARSE_POINT,
	0,  // Determined by extension
	FILE_ATTRIBUTE_SYSTEM,
	FILE_ATTRIBUTE_VIRTUAL,
	FILE_ATTRIBUTE_DEVICE
};

// Output colors for filetypes
constexpr WORD FILECOLOR[] = {
	0,
	0,
	FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_GREEN,
};
