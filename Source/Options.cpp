#include "stdafx.h"
#include "Options.h"

void AddArgument(Option &&Opt) {
	::Options.push_back(std::move(Opt));
	::Flags += Opt.Flag;
}

void GetOpts(const char *Argument) {
	size_t Pos;
	for (int i = 1; Argument[i] != 0; i++) {
		Pos = ::Flags.find(Argument[i]);
		if (Pos != std::string::npos) {
			::Options[Pos].Target = ::Options[Pos].Value;
		}
	}
}

void Help() {
	std::cout << "Usage:\tlst [" << ::Flags << "] [file ...]" << std::endl;
	std::cout << "Arguments:" << std::endl;
	for (const Option &Opt : ::Options) {
		std::cout << Opt.Flag << '\t' << Opt.Description << std::endl;
	}
}
