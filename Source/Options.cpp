#include "stdafx.h"
#include "Options.h"

void AddArgument(Option &&Opt) {
	::Options.push_back(std::move(Opt));
	::Flags += Opt.Flag;
}

void GetOpts(const TCHAR *Argument) {
	for (int i = 1; Argument[i] != 0; i++) {
		const std::size_t Pos = ::Flags.find(Argument[i]);
		if (Pos != tstring::npos) {
			::Options[Pos].Target = ::Options[Pos].Value;
		}
	}
}

void Help() {
	tcout << _T("Usage:\tlst [") << ::Flags << _T("] [file]...") << std::endl;
	tcout << _T("Arguments:") << std::endl;
	for (const Option &Opt : ::Options) {
		tcout << Opt.Flag << _T('\t') << Opt.Description << std::endl;
	}
}
