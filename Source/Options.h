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
	std::vector<Option> Options;
	std::string Flags;
}


void AddArgument(Option &&Opt);

void GetOpts(const char *Argument);

void Help();
