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
	tstring Flags;
}

void AddArgument(Option &&Opt);

void GetOpts(const TCHAR *Argument);

void Help();
