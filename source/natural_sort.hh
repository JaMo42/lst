#pragma once
#include "stdafx.hh"
#include "unicode.hh"

def natural_compare (std::string_view a, std::string_view b) -> int;

static inline def natural_compare (const fs::path &a, const fs::path &b) -> int
{
  let const astr = unicode::path_to_str (a);
  let const bstr = unicode::path_to_str (b);
  return natural_compare (std::string_view {astr.begin (), astr.end ()},
                          std::string_view {bstr.begin (), bstr.end ()});
}
