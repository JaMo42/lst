// Compile-time options
#pragma once

//////////////////////////////////////////////////////////////////////////
// Colors

// Color for the '<DIR>' size field in long listings
constexpr const char *dir_size_color = "\x1b[38;5;61m";

// Color for the file size field in long listings
constexpr const char *file_size_color = "\x1b[38;5;110m";

// Color for owner and group names in long listings
constexpr const char *name_color = "\x1b[37m";

// Color for fields that could not be evluated and are replaced with a ?
constexpr const char *error_color = "\x1b[31m";

// Color for file names of files for which std::filesystem::status failed
constexpr const char *file_name_error_color = "\x1b[33m";

//////////////////////////////////////////////////////////////////////////
// Display

// Use octal escape sequences for unprintable ascii characters
constexpr bool ascii_escape_octal = false;

//////////////////////////////////////////////////////////////////////////
// Behaviour

// The default string for the long output format
// ("$t$p $l $o $g $s $d $n" mimics GNU ls)
constexpr std::string_view default_long_output_format = "$t$p $l $o $g $s $d $n"sv;
