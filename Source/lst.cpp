#include "stdafx.h"
#include "lst.h"
#include "Unicode.h"


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
			for (const TCHAR &C : FileName) {
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
	// Cache for previous output colors
	WORD PrevColor;
}


void PreserveCurrentColor() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	ZeroMemory(&csbi, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	GetConsoleScreenBufferInfo(::HCon, &csbi);
	::PrevColor = csbi.wAttributes;
}

namespace {
/**
 * @brief Get the owner and group SID of a file.
 * @param File - File handle.
 * @return a {owner sid, group sid} pair.
 */
std::pair<tstring, tstring> GetOwnerAndGroup(const FileInfo &File) {
	PSID Owner, Group;
	std::pair<tstring, tstring> Return;
	// ******** Get the owner and group PSID ********
	PSECURITY_DESCRIPTOR psd;
	if (GetSecurityInfo(
		File.Handle,
		SE_FILE_OBJECT,
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
		&Owner,
		&Group,
		NULL,
		NULL,
		&psd) != ERROR_SUCCESS) {
		return {_T("?"), _T("?")};
	}
	LPTSTR OwnerName = NULL, GroupName = NULL, DomainName = NULL;
	DWORD OwnerSize = 0, GroupSize = 0, DomainSize = 0;
	// ******** Get the owner and group names ********
	SID_NAME_USE Use = SidTypeUnknown;
	// Get size for account name
	if (LookupAccountSid(
		NULL, Owner,
		OwnerName, &OwnerSize,
		DomainName, &DomainSize,
		&Use
	) != ERROR_SUCCESS) {
		const DWORD Error = GetLastError();
		if (Error != ERROR_INSUFFICIENT_BUFFER) {
			Return.first = _T("?");
		}
	} else {
		// Allocate account name
		OwnerName = (LPTSTR)std::malloc(OwnerSize * sizeof(TCHAR));
		// Get account name
		LookupAccountSid(
			NULL, Owner,
			OwnerName, &OwnerSize,
			DomainName, &DomainSize,
			&Use
		);
		Return.first = OwnerName;
		std::free(OwnerName);
	}
	// Get size for group name
	if (LookupAccountSid(
		NULL, Owner,
		GroupName, &GroupSize,
		DomainName, &DomainSize,
		&Use
	) != ERROR_SUCCESS) {
		const DWORD Error = GetLastError();
		if (Error != ERROR_INSUFFICIENT_BUFFER) {
			Return.second = _T("?");
		}
	} else {
		// Allocate group name
		GroupName = (LPTSTR)std::malloc(GroupSize * sizeof(TCHAR));
		// Get group name
		LookupAccountSid(
			NULL, Owner,
			GroupName, &GroupSize,
			DomainName, &DomainSize,
			&Use
		);
		Return.second = GroupName;
		std::free(GroupName);
	}
	return Return;
}

tstring FormatSize(DWORD Size) {
	tstring sizes[] = {_T("B"), _T("KB"), _T("MB"), _T("GB"), _T("TB"), _T("PB")};
	int order = 0;
	while (Size >= 1024 && order < 5) {
		++order;
		Size /= 1024;
	}
	return to_tstring(Size) + sizes[order];
}

tstring FormatTime(const FILETIME& FileTime) {
	SYSTEMTIME Time = {0};
	FileTimeToSystemTime(&FileTime, &Time);
	//Jan 13 07:11
	TCHAR TimeBuf[13] = {0};
	// Convert SYSTEMTIME to std::tm
	std::tm timeinfo = {0};
	// Convert required fields
	timeinfo.tm_hour = Time.wHour;
	timeinfo.tm_mday = Time.wDay;
	timeinfo.tm_min = Time.wMinute;
	timeinfo.tm_mon = Time.wMonth - 1;
	// std::tm holds years since 1900 while SYSTEMTIME holds the current year
	timeinfo.tm_year = Time.wYear - 1900;
	// Format using strftime
	tstrftime(TimeBuf, 13, _T("%h %e %R"), &timeinfo);
	return TimeBuf;
}

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


void OutputFileLong(const FileInfo &File, int Color, int Quoting, int Indicator, int NameLength) {
	// Get number of links
	// @TODO
	// This is kind of broken,as it only gives one link for directories (being ..) and no others
	// I tried using std::filesystem but calling std::filesystem::hardlink_count just crashed the program.
	BY_HANDLE_FILE_INFORMATION fi = {0};
	GetFileInformationByHandle(File.Handle, &fi);
	// Get owner and group
	const auto [Owner, Group] = ::GetOwnerAndGroup(File);
	// Get the size
	const tstring Size = ::FormatSize(File.Size);
	// Get the modification time
	const tstring Time = ::FormatTime(File.LastWrite);
	// Print information
	// %s %02d %02d:%02d
	tprintf(_T("%c %3d %*.*s %*.*s %6s %s "),
		FiletypeChars[(unsigned)File.Type],
		fi.nNumberOfLinks,
		NameLength, NameLength, Owner.c_str(),
		NameLength, NameLength, Group.c_str(),
		Size.c_str(),
		Time.c_str()
	);
	OutputFile(File, Color, Quoting, Indicator);
	if (File.LinkOK) {
		tcout << " -> " << File.LinkName;
	}
	tcout << std::endl;
}
