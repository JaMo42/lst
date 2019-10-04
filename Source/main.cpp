#include "stdafx.h"
#include "Unicode.h"
#include "lst.h"
#include "Options.h"

namespace Options {
	int ListDir = false;
	int Sorting = SORT_NAME;
	int ReverseSorting = false;
	int Indicators = false;
	int ShowAll = false;
	int Quoting = 0;
	int Color = true;
	int NewLine = true;
}

int _tmain(const int argc, const TCHAR *argv[]) {
	// Setup arguments
#define O(f, d, t, v) {  _T(f), _T(d), t, v }
	AddArgument(O('a', "Show hidden files and files starting with `.`.", Options::ShowAll, 1));
	AddArgument(O('d', "If argument is directory, list it, not its contents.", Options::ListDir, true));
	AddArgument(O('m', "Do not colorize the output.", Options::Color, false));
	AddArgument(O('n', "Do not sort the listing.", Options::Sorting, SORT_NONE));
	AddArgument(O('q', "Always quote file names.", Options::Quoting, 1));
	AddArgument(O('r', "Reverse sorting order.", Options::ReverseSorting, true));
	AddArgument(O('s', "Sort by file size.", Options::Sorting, SORT_SIZE));
	AddArgument(O('t', "Sort by time modified.", Options::Sorting, SORT_MODIFIED));
	//AddArgument(O('A', "Like `a` but do not list implied `.` and `..`.", Options::ShowAll, 2)); @TODO
	AddArgument(O('F', "Add a `\\` after all directory names.", Options::Indicators, true));
	AddArgument(O('L', "Seperate files using spaces instead of newlines.", Options::NewLine, true));
	AddArgument(O('Q', "Never quote file names.", Options::Quoting, 2));
	AddArgument(O('T', "Sort by time created.", Options::Sorting, SORT_CREATED));
#undef O
	// Get options
	std::vector<tstring> FileNames{};
	// No arguments were passed, list current directory
	tstring Flags = _T("");
	if (argc == 1) {
		TCHAR buf[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buf);
		FileNames.push_back({ buf });
		goto SkipArgs;
	} else {
		for (int i = 1; i < argc; i++) {
			if (tstrcmp(argv[i], _T("--help")) == 0) {
				Help();
				exit(0);
			} else if (argv[i][0] == _T('-')) {
				GetOpts(argv[i]);
			} else {
				FileNames.push_back({ argv[i] });
			}
		}
	}
SkipArgs:
	std::vector<FileInfo> Content;
	bool IsDirectory;
	if (Options::Color)
		PreserveCurrentColor();
	// Set seperator, always use newline for long listing
	const TCHAR Seperator = Options::NewLine ? _T('\n') : _T(' ');
	for (tstring &FileName : FileNames) {
		// Check if the file is a directory
		IsDirectory = GetFileAttributes(FileName.c_str()) & FILE_ATTRIBUTE_DIRECTORY;
		// If listing directory contents, and the item is a directory, append
		// wildcard to the end, this is required to list the contents of the
		// directory and not the directory itself.
		if (IsDirectory && !Options::ListDir)
			FileName += _T("\\*");
		// List
		if (SUCCEEDED(List(FileName, Content))) {
			// Sort
			SortDir(Content, Options::Sorting);
			// Maybe reverse
			if (Options::ReverseSorting)
				std::reverse(Content.begin(), Content.end());
			// Output
			for (const FileInfo &File : Content) {
				// Maybe skip file
				if ((Options::ShowAll == 0) && File.Hidden)
					continue;
				// Output file
				OutputFile(File, Options::Color, Options::Quoting, Options::Indicators);
				std::cout << Seperator;
			}
		} else {
			tcout << _T("Could not list ") << FileName << _T('\n');
		}
	}
}
