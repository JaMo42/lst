// Compile-time options
#pragma once

//////////////////////////////////////////////////////////////////////////
// Colors

// Color for all other text that doesn't have a special color
constexpr const char *text_color = "\x1b[39m";

// Color for the '<DIR>' size field in long listings
constexpr const char *dir_size_color = "\x1b[38;5;61m";

// Color for the file size field in long listings
constexpr const char *file_size_color = "\x1b[38;5;110m";

// Color for owner and group names in long listings
constexpr const char *name_color = "\x1b[37m";

// Color for fields that could not be evluated and are replaced with a ?
constexpr const char *error_color = "\x1b[31m";

// Color for file names of files for which std::filesystem::status failed
constexpr const char *file_name_error_color = "\x1b[39m";

//////////////////////////////////////////////////////////////////////////
// Display

// Use octal escape sequences for unprintable ascii characters
constexpr bool ascii_escape_octal = false;

// Units for --si
constexpr const char *units_1000[] = { "k", "M", "G", "T", "P", "E" };
//constexpr const char *units_1000[] = { "KB", "MB", "GB", "TB", "PB", "EB" };

// Units for -h
constexpr const char *units_1024[] = { "K", "M", "G", "T", "P", "E" };
//constexpr const char *units_1024[] = { "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };

//////////////////////////////////////////////////////////////////////////
// Behaviour

// The default string for the long output format
// ("$t$p $l $o $g $s $d $n" mimics GNU ls)
constexpr std::string_view default_long_output_format = "$t$p $l $o $g $s $d $n"sv;
