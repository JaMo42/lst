#pragma once
#include "stdafx.h"
#include "Unicode.h"

struct Option {
	const TCHAR Flag;
	const TCHAR *Description;
	int &Target;
	int Value;
};

namespace {
    // Holds all arguments added by AddArgument
	std::vector<Option> Options;
    // Contains all flags for the arguments, used by GetOpts
	tstring Flags;
}

/**
 * @brief Adds a commandline option.
 * @param Opt - The option to add.
 */
void AddArgument(Option &&Opt);

/**
 * @brief Applies all options given in a commandline argument.
 * For each given flag, sets the target of the option to its specified value.
 */ 
void GetOpts(const TCHAR *Argument);

/**
 * @brief Displays the help text.
 */ 
void Help();
