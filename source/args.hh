#pragma once
#include "stdafx.hh"

enum class SortMode
{
  none,
  name,
  extension,
  size,
  time,
  version
};

enum class QuoteMode
{
 default_,
 literal,
 double_
};

enum class NongraphicMode
{
  escape,
  hide,
  show
};

enum class TimeMode
{
  access,
  write,
  creation
};

struct LongColumn
{
  using Enum = enum
  {
    type_indicator,
    rwx_perms,
    oct_perms,
    hard_link_count,
    owner_name,
    group_name,  // Domain name on Windows
    size,
    date,
    name,
    text
  };
  static constexpr std::string_view field_chars = "tpPlogsdn"sv;

  LongColumn (Enum value)
    : M_value (value)
    , M_text_text {}
  {}

  LongColumn (std::string_view text_text)
    : M_value (text)
    , M_text_text (text_text)
  {}

  operator Enum () const { return M_value; }

  std::string_view get_text () const { return M_text_text; }

private:
  Enum M_value;
  // The text corresponding to the 'text' enum value
  std::string_view M_text_text;
};

extern bool G_is_a_tty;

extern const char *G_program;

namespace Arguments
{
extern bool all;
extern bool long_listing;
extern bool single_column;
extern bool color;
extern bool recursive;
extern bool classify;
extern bool file_type;
extern bool immediate_dirs;
extern bool reverse;
extern SortMode sort_mode;
extern QuoteMode quoting;
extern NongraphicMode nongraphic;
extern unsigned human_readble;
extern unsigned width;
extern bool english_errors;
extern bool group_directories_first;
extern arena::vector<LongColumn> long_columns;
extern std::bitset<LongColumn::text + 1> long_columns_has;
extern bool case_sensitive;
extern bool ignore_backups;
extern bool dereference;
extern TimeMode time_mode;
}

def parse_args (int argc, const char **argv,
                arena::vector<fs::path> &args) -> bool;

def parse_long_format (std::string_view format, arena::vector<LongColumn> &out) -> bool;
