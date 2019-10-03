#pragma once
#include "stdafx.h"
#include "FileInfo.h"

// Sorting modes
enum {
	SORT_NONE,
	SORT_NAME,
	SORT_SIZE,
	SORT_MODIFIED,
	SORT_CREATED
};

/**
 * @brief Lists a directory or file
 * @param Directory - The name of the directory or file
 * @param Content - Output for the content
 */
HRESULT List(const tstring &Directory, std::vector<FileInfo> &Content);

/**
 * @brief Sorts directory contents
 * @param Dir - The directory to sort
 * @param SortingMode  The sorting mode
 */
void SortDir(std::vector<FileInfo> &Dir, int SortingMode);

/**
 * @brief Stores the current color of the console
 * Used to reset colors faster
 */
void PreserveCurrentColor();

/**
 * @brief Output a file wiht various formatting
 * @param File - The file to output
 * @param Color - Whether to colorize the output or not
 * @param Quoting - The quoting style
 * @param Indicator - Whether to put indicators behind certain files
 */
void Output(const FileInfo &File, int Color, int Quoting, int Indicator);
