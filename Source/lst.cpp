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

tstring Quote(const tstring &FileName, int Quoting) {
	constexpr const char *SpecialCharacters = " '\"";
	switch (Quoting) {
		case 0:
			for (const char &C : FileName) {
				if (strchr(SpecialCharacters, C)) {
					goto HasSpecial;
				}
			}
			return FileName;
			HasSpecial:
		case 1:
			if (FileName.find("\"") != std::string::npos)
				return '\'' + FileName + '\'';
			else
				return '"' + FileName + '"';
		case 2:
			return FileName;
	}
}

namespace {
	const static HANDLE HCon = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD PrevColor;
}

void PreserveCurrentColor() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	ZeroMemory(&csbi, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	GetConsoleScreenBufferInfo(::HCon, &csbi);
	::PrevColor = csbi.wAttributes;
}

void Output(const FileInfo &File, int Color, int Quoting, int Indicator) {
	const WORD FileColor = FILECOLOR[(int)File.Type];
	if (Color && FileColor != 0)
		SetConsoleTextAttribute(::HCon, FileColor);
	std::cout << Quote(File.Name, Quoting);
	SetConsoleTextAttribute(::HCon, ::PrevColor);
	if (Indicator && File.Type == FILETYPE::Directory) {
		std::cout << '\\';
	}
}
