#include "stdafx.h"
#include "Unicode.h"
#include "lst.h"
#include "Options.h"

namespace Options {
    // List directories themselves, not their contents
	int ListDir = false;
    // Sorting mode
	int Sorting = SORT_NAME;
    // Whether to reversee the sorting order
	int ReverseSorting = false;
    // Whther to add incicators behind specific files (curently only \ after directories)
	int Indicators = false;
    // Whether to show all files
	int ShowAll = false;
    // The quoting mode
	int Quoting = QUOTE_AUTO;
    // Whther to show colors or not
	int Color = true;
    // Whether to seperate entries by newline instead of spaces
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
	AddArgument(O('F', "Add a `\\` after all directory names.", Options::Indicators, true));
	AddArgument(O('L', "Seperate files using spaces instead of newlines.", Options::NewLine, true));
	AddArgument(O('Q', "Never quote file names.", Options::Quoting, 2));
	AddArgument(O('T', "Sort by time created.", Options::Sorting, SORT_CREATED));
#undef O
	// Get options
	std::vector<tstring> FileNames{};
	// Parse the arguments
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
	// If no files were specified, add the current directory
	if (FileNames.empty()) {
		TCHAR buf[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buf);
		FileNames.push_back({buf});
	}
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
		// Otherwise the reiling [back]slash, if present, must be removed, in order
		// to list the directory itself.
		if (IsDirectory) {
			if (!Options::ListDir) {
				FileName += _T("\\*");
			} else if (FileName.back() == '/' || FileName.back() == '\\') {
				FileName.pop_back();
			}
		}
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
