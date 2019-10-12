#include "stdafx.h"
#include "lst.h"

HRESULT List(const tstring &Directory, std::vector<FileInfo> &Content) {
	HANDLE Find;
	WIN32_FIND_DATA FindData;
	Find = FindFirstFile(Directory.c_str(), &FindData);
	if (Find == INVALID_HANDLE_VALUE) {
		return E_FAIL;
	}
	Content.push_back(FileInfo(FindData));
	while (FindNextFile(Find, &FindData)) {
		Content.push_back(FileInfo(FindData));
	}
	return S_OK;
}

void SortDir(std::vector<FileInfo> &Dir, int SortingMode) {
#define SORT(comp) std::sort(Dir.begin(), Dir.end(), [](const FileInfo &a, const FileInfo &b) -> bool comp)
	switch (SortingMode) {
		case SORT_NONE:
			return;
		case SORT_NAME:
			SORT({
				return a.Name < b.Name;
			});
			break;
		case SORT_SIZE:
			SORT({
				return a.Size < b.Size;
			});
			break;
		case SORT_MODIFIED:
			SORT({
				return CompareFileTime(&a.LastWrite, &b.LastWrite) == -1 ? true : false;
			});
			break;
		case SORT_CREATED:
			SORT({
				return CompareFileTime(&a.Creation, &b.Creation) == -1 ? true : false;
			});
			break;
	}
#undef SORT
}

/**
 * @brief Adds quoting to a file name
 * @param FileName - The name of the file
 * @param Quoting - The quoting mode
 * @return The quoted file name
 */
inline tstring Quote(const tstring &FileName, int Quoting) {
	constexpr const char *SpecialCharacters = " '\"";
	switch (Quoting) {
		case QUOTE_AUTO:
			for (const char &C : FileName) {
				if (strchr(SpecialCharacters, C)) {
					goto HasSpecial;
				}
			}
			return FileName;
			HasSpecial:
		case QUOTE_ALWAYS:
			if (FileName.find(_T("\"")) != std::string::npos)
				return _T('\'') + FileName + _T('\'');
			else
				return _T('"') + FileName + _T('"');
		case QUOTE_NEVER:
			return FileName;
	}
}


namespace {
	// Handle to stdout
	const static HANDLE HCon = GetStdHandle(STD_OUTPUT_HANDLE);
	// Cache for output colors
	WORD PrevColor;
}

void PreserveCurrentColor() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	ZeroMemory(&csbi, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	GetConsoleScreenBufferInfo(::HCon, &csbi);
	::PrevColor = csbi.wAttributes;
}

void OutputFile(const FileInfo &File, int Color, int Quoting, int Indicator) {
	const WORD FileColor = FILECOLOR[(int)File.Type];
	if (Color && FileColor != 0)
		SetConsoleTextAttribute(::HCon, FileColor);
	tcout << Quote(File.Name, Quoting);
	SetConsoleTextAttribute(::HCon, ::PrevColor);
	if (Indicator && File.Type == FILETYPE::Directory) {
		tcout << _T('\\');
	}
}
