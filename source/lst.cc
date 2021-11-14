#include "lst.hh"
#include "columns.hh"

#ifdef _WIN32
static const fs::path S_lnk_ext { L".lnk"s };
static const fs::path S_exe_ext { L".exe"s };
static const fs::path S_bat_ext { L".bat"s };
static const fs::path S_cmd_ext { L".cmd"s };
static const fs::path S_bak_ext { L".bak"s };
static const fs::path S_tmp_ext { L".tmp"s };
static const fs::path S_hiberfil { L"hiberfil.sys"s };
static const fs::path S_swapfile { L"swapfile.sys"s };
#else
static const fs::path S_bak_ext { ".bak"s };
static const fs::path S_tmp_ext { ".tmp"s };
#endif

std::time_t G_six_months_ago;

FileList G_singles;
std::list<std::pair<fs::path, FileList>> G_directories;

static std::error_code S_ec;
static arena::vector<int> S_did_complain_about;


static def complain (const fs::path &file)
{
  if (std::find (S_did_complain_about.begin (), S_did_complain_about.end (),
                 S_ec.value ()) == S_did_complain_about.end ())
    {
#ifdef _WIN32
      if (Arguments::english_errors)
        {
          LPSTR msg;
          FormatMessageA ((FORMAT_MESSAGE_ALLOCATE_BUFFER
                           | FORMAT_MESSAGE_FROM_SYSTEM
                           | FORMAT_MESSAGE_IGNORE_INSERTS),
                          NULL,
                          S_ec.value (),
                          MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US),
                          (LPSTR)&msg,
                          0,
                          NULL);
          std::fprintf (stderr, "%s: %s: %s", G_program, file.string ().c_str (),
                        msg);
          LocalFree (msg);
        }
      else
#endif
        {
          std::fprintf (stderr, "%s: %s: %s\n", G_program, file.string ().c_str (),
                        S_ec.message ().c_str ());
        }
      S_did_complain_about.push_back (S_ec.value ());
    }
  S_ec.clear ();
}


static def escape_nongraphic (char32_t c, arena::string &out)
{
  // TODO
  // maybe don't use escape sequences for unicode but 'U+XXXX' style sequences
  // with a different color (will need extra handling for file width, like
  // pre-calculating the width or a width offset in add_frills)
  // ^ if doing this, use color for ascii escapes as well.

  static constexpr char digit_chars[] = "0123456789ABCDEF";
  out.push_back ('\\');
  if (c < 0x80)
    {
      if constexpr (ascii_escape_octal)
        {
          out.push_back ('0' + c / 64);
          out.push_back ('0' + (c / 8) % 8);
          out.push_back ('0' + c % 8);
        }
      else
        {
          out.push_back ('x');
          out.push_back (digit_chars[c / 16]);
          out.push_back (digit_chars[c % 16]);
        }
    }
  else if (c < 0x10000)
    {
      out.push_back ('u');
      out.push_back (digit_chars[c / 0x1000]);
      out.push_back (digit_chars[(c / 0x100) % 0x10]);
      out.push_back (digit_chars[(c / 0x10) % 0x10]);
      out.push_back (digit_chars[c % 0x10]);
    }
  else
    {
      out.push_back ('U');
      out.push_back (digit_chars[c / 0x10000000]);
      out.push_back (digit_chars[(c / 0x1000000) % 0x10]);
      out.push_back (digit_chars[(c / 0x100000) % 0x10]);
      out.push_back (digit_chars[(c / 0x10000) % 0x10]);
      out.push_back (digit_chars[(c / 0x1000) % 0x10]);
      out.push_back (digit_chars[(c / 0x100) % 0x10]);
      out.push_back (digit_chars[(c / 0x10) % 0x10]);
      out.push_back (digit_chars[c % 0x10]);
    }
}


static def add_frills (arena::string &&str, arena::string &out)
{
  let const loc = std::locale ("");
  let const always_quote = Arguments::quoting == QuoteMode::double_;
  let need_quoting = false;
  let quote_char = always_quote ? '"' :  U'\0';
  let p = str.c_str ();
  char32_t c;
  int cp_size = 0;

  out.reserve (str.size () * 1.2);

  // Source: https://www.opencoverage.net/coreutils/index_html/source_213.html
  // Quote the name if it starts with any of there
  let static constexpr quote_if_first = "#~"sv;
  // Quote the name if it contains any of these
  let static constexpr quote_if_anywhere = " !$&()*;<=>[^`|"sv;

  // Pre-process for quotation marks since we need to know ahead of time if we
  // weill need to escape them in the second loop. We do not need to care about
  // unicode in this loop.
  for (let const c : str)
    {
      if (c == '\'' && !quote_char) // Prefer single quotes if the name contains both styles
        quote_char = '"';
      else if (c == '"' && !always_quote)
        quote_char = '\'';
      else if (quote_if_anywhere.find (c) != std::string_view::npos)
        need_quoting = true;
      // TODO: there are probably more cases that need quoting
      // (also platform dependency)
    }
  // Numbner sign only requires outing if it is the first character (technically
  // the first non-whitespace character but if there is whitespace before this
  // we already quote anyways)
  if (!need_quoting && quote_if_first.find (str[0]) != std::string_view::npos)
    need_quoting = true;
  // These may be special if isolated (line 525 in the above mention source)
  else if (str == "{" || str == "}")
    need_quoting = true;

  if (need_quoting && !quote_char)
    quote_char = '\'';

  if (quote_char)
    out.push_back (quote_char);

  let push_code_point = [&cp_size, &p, &out]() {
    for (let i = 0; i < cp_size; ++i)
      out.push_back (p[i]);
  };

  for (std::size_t i = 0; i < str.size (); i += cp_size, p += cp_size)
    {
      c = unicode::utf8_to_codepoint (p, &cp_size);

      if (c == quote_char)
        out.push_back ('\\');
      // TODO: the locale version of std::isprint returns false for '파' but
      // true for '일'?, this solution makes the handling of unicode sequences
      // in escape_nongraphic pointless.
      //else if (!std::isprint (c, loc))
      else if (c < 0x80 && !std::isprint (c))
        {
          switch (Arguments::nongraphic)
            {
              case NongraphicMode::escape:
                escape_nongraphic (c, out);
                break;
              case NongraphicMode::hide:
                out.push_back ('?');
                break;
              case NongraphicMode::show:
                push_code_point ();
                break;
            }
        }
      else
        push_code_point ();
    }

  if (quote_char)
    out.push_back (quote_char);
}


FileInfo::FileInfo (const fs::path &p, const fs::file_status &s, link_target_tag)
  : _path (p.filename ())
{
  add_frills (unicode::path_to_str (p.filename ()), name);

  let const ext = p.extension ();
#ifdef _WIN32
  if (ext == S_exe_ext || ext == S_bat_ext || ext == S_cmd_ext)
    type = FileType::executable;
  else if (ext == S_lnk_ext)
    type = FileType::symlink;
  else
#endif
  type = file_type (s);
}


FileInfo::FileInfo (const fs::path &p, const fs::file_status &s)
  : _path (p.filename ())
{
  // TODO: Maybe handle dereferencing here
  S_did_complain_about.clear ();

  add_frills (unicode::path_to_str (p.filename ()), name);

  let const ext = p.extension ();

  if (Arguments::long_listing)
    {
      if (!get_owner_and_group (fs::absolute (p), owner, group))
        {
#ifdef _WIN32
          S_ec = std::error_code (GetLastError (), std::system_category ());
#else
          S_ec = std::error_code (errno, std::system_category ());
#endif
          complain (p);
        }

      size = fs::is_directory (s)  ? 0 : fs::file_size (p, S_ec);
      if (S_ec)
        {
          complain (p);
          size = 0;
        }

      let const t = fs::last_write_time (p, S_ec);
      if (S_ec)
        {
          complain (p);
          time = 0;
        }
      time = file_time_to_time_t (t);

      link_count = fs::hard_link_count (p, S_ec);
      if (S_ec)
        {
          complain (p);
          link_count = 0;
        }

      perms = s.permissions ();

      if (s.type () == fs::file_type::symlink)
        {
          let const tp = fs::read_symlink (p);
          target = new FileInfo (tp, fs::status (tp), link_target_tag {});
        }
#ifdef _WIN32
      else if (G_has_shortcut_interfaces && ext == S_lnk_ext)
        {
          arena::string target_name;
          get_shortcut_target (p, target_name);
          let const tp = fs::path (target_name);
          target = new FileInfo (tp, fs::status (tp), link_target_tag {});
        }
#endif
    }
  else
    {
      // Only get extra information needed for sorting
      switch (Arguments::sort_mode)
        {
          case SortMode::size:
            size = fs::file_size (p);
            break;
          case SortMode::time:
            time = file_time_to_time_t (fs::last_write_time (p));
            break;
          // All other sorting modes just need the name
          default:;
        }
    }

#ifdef _WIN32
  if (ext == S_exe_ext || ext == S_bat_ext || ext == S_cmd_ext)
    type = FileType::executable;
  else if (target || ext == S_lnk_ext)
    type = FileType::symlink;
  else
#endif
  type = file_type (s);
}


FileInfo::~FileInfo ()
{
  delete target;
}


def list_file (const fs::path &path) -> void
{
  G_singles.emplace_back (path, fs::status (path));
}


def list_dir (const fs::path &path) -> void
{
  FileList *l = &G_directories.emplace_back (std::make_pair (path, FileList {})).second;

  for (let e : fs::directory_iterator (path))
    {
      if (!Arguments::all
          && e.path ().filename ().c_str ()[0] == std::filesystem::path::value_type {'.'})
        continue;
      // TODO: almost_all

      l->emplace_back (e.path (), e.status ());

      if (Arguments::recursive && e.is_directory ())
        list_dir (e.path ());
    }
}

#ifdef _WIN32

IShellLink *G_sl = nullptr;
IPersistFile *G_pf = nullptr;
bool G_has_shortcut_interfaces = true;

def get_owner_and_group (const fs::path &path, arena::string &owner_out,
                         arena::string &group_out) -> bool
{
  PSID owner_sid;
  PSECURITY_DESCRIPTOR sd;
  SID_NAME_USE use = SidTypeUnknown;
  DWORD owner_size = 1, group_size = 1;
  HANDLE file_handle;

  file_handle = CreateFileW (path.wstring ().c_str (),
                             GENERIC_READ,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             nullptr);
  if (file_handle == INVALID_HANDLE_VALUE)
    {
      owner_out.assign ("?");
      group_out.assign ("?");
      return false;
    }

  if (GetSecurityInfo (file_handle, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                       &owner_sid, NULL, NULL, NULL, &sd)
      != ERROR_SUCCESS)
    {
      owner_out.assign ("?");
      group_out.assign ("?");
      return false;
    }

  // first call for buffer sizes
  if (!LookupAccountSidA (NULL, owner_sid, NULL, (LPDWORD)&owner_size,
                          NULL, (LPDWORD)&group_size, &use)
      && GetLastError () != ERROR_INSUFFICIENT_BUFFER)
    {
      owner_out.assign ("?");
      group_out.assign ("?");
      return false;
    }

  owner_out.resize (owner_size);
  group_out.resize (group_size);

  if (!LookupAccountSidA (NULL, owner_sid, owner_out.data (), &owner_size,
                          group_out.data (), &group_size, &use))
    {
      owner_out.assign ("?");
      group_out.assign ("?");
      return false;
    }

  return true;
}


#define CHECK(x) if (FAILED (x)) { target_out.assign ("?"); return false; }

def get_shortcut_target (const fs::path &path, arena::string &target_out) -> bool
{
  let static const size = MAX_PATH;
  static char path_buf[size];

  if (!G_has_shortcut_interfaces)
    {
      target_out.assign ("?");
      return false;
    }

  CHECK (G_pf->Load (path.wstring ().c_str (), STGM_READ));
  CHECK (G_sl->Resolve (nullptr, 0));
  CHECK (G_sl->GetPath (path_buf, size, nullptr, 0));

  target_out.assign (path_buf);
  return true;
}

#undef CHECK

#else // _WIN32

def get_owner_and_group (const fs::path &path, arena::string &owner_out,
                         arena::string &group_out) -> bool
{
  struct stat sb {0};
  struct passwd *pw = nullptr;
  struct group *grp = nullptr;

  if (stat (path.string ().c_str (), &sb) == -1)
    {
      owner_out.assign ("?");
      group_out.assign ("?");
      return false;
    }

  pw = getpwuid (sb.st_uid);
  grp = getgrgid (sb.st_gid);

  owner_out.assign (pw->pw_name);
  group_out.assign (grp->gr_name);

  return true;
}

#endif // _WIN32

def file_type (const fs::file_status &s) -> FileType
{
  switch (s.type ())
    {
      case fs::file_type::regular:
        {
#ifndef _WIN32
          let const p = s.permissions ();
          if ((p & fs::perms::owner_exec) != fs::perms::none
              || (p & fs::perms::group_exec) != fs::perms::none
              || (p & fs::perms::others_exec) != fs::perms::none)
            return FileType::executable;
#endif
          return FileType::regular;
        }
      case fs::file_type::directory:
        return FileType::directory;
      case fs::file_type::symlink:
        return FileType::symlink;
      case fs::file_type::block:
        return FileType::block;
      case fs::file_type::character:
        return FileType::character;
      case fs::file_type::fifo:
        return FileType::fifo;
      case fs::file_type::socket:
        return FileType::socket;
      default:
        return FileType::unknown;
    }
}


def file_time_to_time_t (fs::file_time_type ft) -> std::time_t
{
  using namespace std::chrono;
  auto sctp = time_point_cast<system_clock::duration>(ft - fs::file_time_type::clock::now()
    + system_clock::now());
  return system_clock::to_time_t(sctp);
}


static int
case_insensitive_compare (const fs::path &a, const fs::path &b)
{
  let const &astr = a.native ();
  let const &bstr = b.native ();
  let const l = std::min (astr.size (), bstr.size ());
  fs::path::value_type c1, c2;

  for (std::size_t i = 0; i < l; ++i)
    {
      c1 = static_cast<char32_t> (astr[i]) < 0x80 ? std::tolower (astr[i]) : astr[i];
      c2 = static_cast<char32_t> (bstr[i]) < 0x80 ? std::tolower (bstr[i]) : bstr[i];

      if (c1 < c2)
        return -1;
      else if (c1 > c2)
        return 1;
    }
  return astr.size () - bstr.size ();
}


def sort_files (FileList &files) -> void
{
  static std::function<bool (const FileInfo &, const FileInfo &)> sort = nullptr;

  if (Arguments::sort_mode == SortMode::none)
    return;

  #define SORT_FUNC [](const FileInfo &a, const FileInfo &b)
  if (!sort)
    {
      switch (Arguments::sort_mode)
        {
          case SortMode::name:
            if (Arguments::case_sensitive)
              sort = SORT_FUNC {
                return ((a._path < b._path) ^ Arguments::reverse);
              };
            else
              sort = SORT_FUNC {
                return (!case_insensitive_compare (a._path, b._path)
                        ^ Arguments::reverse);
              };
            break;
          case SortMode::extension:
            if (Arguments::case_sensitive)
              sort = SORT_FUNC {
                return ((a._path.extension () < b._path.extension ())
                        ^ Arguments::reverse);
              };
            else
              sort = SORT_FUNC {
                return (!case_insensitive_compare (a._path.extension (),
                                                   b._path.extension ())
                        ^ Arguments::reverse);
              };
            break;
          case SortMode::size:
            sort = SORT_FUNC {
              return (a.size < b.size) ^ Arguments::reverse;
            };
            break;
          case SortMode::time:
            sort = SORT_FUNC {
              return (a.time < b.time) ^ Arguments::reverse;
            };
          case SortMode::version:
            sort = SORT_FUNC {
              // TODO
              (void)a; (void)b;
              return true;
            };
            break;
          case SortMode::none:;
        }
    }
  #undef SORT_FUNC

  files.sort (sort);

  if (Arguments::group_directories_first)
    files.sort ([](const FileInfo &a, const FileInfo &b) {
      return (a.type == FileType::directory) > (b.type == FileType::directory);
    });
}


static def file_color (const FileInfo &f) -> const char *
{
  if (f.name.back () == '~'
      || f._path.extension () == S_bak_ext
      || f._path.extension () == S_tmp_ext)
    return "\x1b[90m";

#if _WIN32
  if (f._path == S_hiberfil
      || f._path == S_swapfile)
    {
      return "\x1b[33m";
    }
#endif

  switch (f.type)
    {
      case FileType::directory:  return "\x1b[94m";
      case FileType::executable: return "\x1b[92m";
      case FileType::symlink:    return "\x1b[96m";
      default:                   return "";
    }
}


def file_indicator (const FileInfo &f) -> char
{
  switch (f.type)
    {
      case FileType::directory:  return '/';
      case FileType::symlink:    return '@';
      case FileType::fifo:       return '|';
      case FileType::socket:     return '=';
      case FileType::executable: return '*';
      default:                   return 0;
    }
}


static def file_indicator_color (const FileInfo &f) -> const char *
{
  switch (f.type)
    {
      case FileType::directory:  return "\x1b[34m";
      case FileType::symlink:    return "\x1b[36m";
      case FileType::fifo:       return "\x1b[90m";
      case FileType::socket:     return "\x1b[90m";
      case FileType::executable: return "\x1b[32m";
      default:                   return nullptr;
    }
}


static def file_name_width (const FileInfo &f) -> int
{
  let w = 0;
  w += unicode::display_width (f.name);

  if (Arguments::classify)
    w += unicode::display_width (file_indicator (f));

  if (f.target)
    {
      w += 4;
      w += unicode::display_width (f.target->name);

      if (Arguments::classify)
        w += unicode::display_width (file_indicator (f));
    }

  return w;
}


def print_file_name (const FileInfo &f, int width) -> void
{
  static arena::string padding_buffer;
  // File name
  if (Arguments::color)
    std::fputs (file_color (f), stdout);
  std::fputs (f.name.c_str (), stdout);
  if (Arguments::color)
    std::fputs ("\x1b[0m", stdout);
  // Indicator
  if (Arguments::classify)
    {
      if (Arguments::color)
        {
          let const color = file_indicator_color (f);
          if (color)
            std::fputs (color, stdout);
        }
      let const indicator = file_indicator (f);
      if (indicator)
        std::putchar (indicator);
      if (Arguments::color)
        std::fputs ("\x1b[0m", stdout);
    }
  // Link target
  if (f.target && (Arguments::long_listing || Arguments::single_column))
    {
      std::fputs (" -> ", stdout);
      print_file_name (*f.target);
    }
  // Padding
  if (width)
    {
      let const w = file_name_width (f);
      if (w >= width)
        return;
      padding_buffer.assign (width - w, ' ');
      const char *padding = padding_buffer.c_str ();
      std::fputs (padding, stdout);
    }
}


static def int_len (std::uintmax_t n) -> int
{
  if (n < 10ULL) return 1;
  if (n < 100ULL) return 2;
  if (n < 1'000ULL) return 3;
  if (n < 10'000ULL) return 4;
  if (n < 100'000ULL) return 5;
  if (n < 1'000'000ULL) return 6;
  if (n < 10'000'000ULL) return 7;
  if (n < 100'000'000ULL) return 8;
  if (n < 1'000'000'000ULL) return 9;
  if (n < 10'000'000'000ULL) return 10;
  if (n < 100'000'000'000ULL) return 11;
  if (n < 1'000'000'000'000ULL) return 12;
  if (n < 10'000'000'000'000ULL) return 13;
  if (n < 100'000'000'000'000ULL) return 14;
  if (n < 1'000'000'000'000'000ULL) return 15;
  return 18;
}


// If 'dry' is true, nothing will be printed but the number of characters that
// would have been written is still returned.
template <bool dry = false>
static def print_size (std::uintmax_t size, unsigned width = 0) -> int
{
  // TODO: some nicer option to set this
#if 1
  // Short units
  static const char *units_1000[] = { "", "k", "M", "G", "T", "P", "E" };
  static const char *units_1024[] = { "", "K", "M", "G", "T", "P", "E" };
#else
  // Long units
  static const char *units_1000[] = { "", "KB", "MB", "GB", "TB", "PB", "EB" };
  static const char *units_1024[] = { "", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };
#endif

  if (Arguments::human_readble)
    {
      let fsize = double (size);
      let p = 0;
      while (fsize > Arguments::human_readble)
        {
          ++p;
          fsize /= Arguments::human_readble;
        }
      if (!p)
        goto no_human;

      let unit = (Arguments::human_readble == 1000 ? units_1000 : units_1024)[p];
      let unit_len = static_cast<int> (std::strlen (unit));

      if constexpr (dry)
        return std::snprintf (nullptr, 0, "%*.1f%s", width, fsize, unit);
      else
        {
          if (Arguments::color)
            return std::printf ("%s%*.1f%s\x1b[0m", file_size_color,
                                width - unit_len, fsize, unit);
          else
            return std::printf ("%*.1f%s", width - unit_len, fsize, unit);
        }
    }
  else
    {
no_human:
      if constexpr (dry)
        return std::snprintf (nullptr, 0, "%llu", size);
      else
        {
          if (Arguments::color)
            return std::printf ("%s%*llu\x1b[0m", file_size_color, width, size);
          else
            return std::printf ("%*llu", width, size);
        }
    }
}


def print_single_column (const FileList &files) -> void
{
  for (let const &f : files)
    {
      print_file_name (f);
      std::putchar ('\n');
    }
}


def print_long (const FileList &files) -> void
{
  arena::string size_buf;
  const int date_sz = 14;
  char date_buf[date_sz];
  bool invalid_time = false;

  int name_width = 0, link_width = 1, owner_width = 1, group_width = 1,
      size_width = 1;

  let const name_is_last = Arguments::long_columns.back () == LongColumn::name;

  // Get column widths
  for (let const &f : files)
    {
      // TODO: Only calculate needed widths
      if (!name_is_last)
        name_width = std::max (name_width, file_name_width (f));
      link_width = std::max (link_width, int_len (f.link_count));
      owner_width = std::max (owner_width, unicode::display_width (f.owner));
      group_width = std::max (group_width, unicode::display_width (f.group));
      if (f.type == FileType::directory)
        size_width = std::max (size_width, 5);
      else
        size_width = std::max (size_width, print_size<true> (f.size));
    }

  for (let const &f : files)
    {
#ifdef _WIN32
      if (f._path == S_hiberfil
          || f._path == S_swapfile)
        {
          invalid_time = true;
        }
#endif
      for (let const &col : Arguments::long_columns)
        {
          switch (static_cast<LongColumn::Enum> (col))
            {
              case LongColumn::type_indicator:
                {
                  std::putchar (file_type_letters[static_cast<int> (f.type)]);
                }
                break;

              case LongColumn::rwx_perms:
                {
                  std::putchar ((f.perms & fs::perms::owner_read) != fs::perms::none ? 'r' : '-');
                  std::putchar ((f.perms & fs::perms::owner_write) != fs::perms::none ? 'w' : '-');
                  std::putchar ((f.perms & fs::perms::owner_exec) != fs::perms::none ? 'x' : '-');
                  std::putchar ((f.perms & fs::perms::group_read) != fs::perms::none ? 'r' : '-');
                  std::putchar ((f.perms & fs::perms::group_write) != fs::perms::none ? 'w' : '-');
                  std::putchar ((f.perms & fs::perms::group_exec) != fs::perms::none ? 'x' : '-');
                  std::putchar ((f.perms & fs::perms::others_read) != fs::perms::none ? 'r' : '-');
                  std::putchar ((f.perms & fs::perms::others_write) != fs::perms::none ? 'w' : '-');
                  std::putchar ((f.perms & fs::perms::others_exec) != fs::perms::none ? 'x' : '-');
                }
                break;

              case LongColumn::oct_perms:
                {
                  let static constexpr owner_mask = (fs::perms::owner_read
                                                    | fs::perms::owner_write
                                                    | fs::perms::owner_exec);
                  let static constexpr group_mask = (fs::perms::group_read
                                                    | fs::perms::group_write
                                                    | fs::perms::group_exec);
                  let static constexpr others_mask = (fs::perms::others_read
                                                    | fs::perms::others_write
                                                    | fs::perms::others_exec);
                  std::printf ("%c%c%c",
                               '0' + (static_cast<int> (f.perms & owner_mask) >> 6),
                               '0' + (static_cast<int> (f.perms & group_mask) >> 3),
                               '0' + (static_cast<int> (f.perms & others_mask)));
                }
                break;

              case LongColumn::hard_link_count:
                {
                  std::printf ("%*d", link_width, f.link_count);
                }
                break;

              case LongColumn::owner_name:
                {
                  if (Arguments::color)
                    std::fputs (f.group == "?"sv
                                  ? error_color
                                  : name_color,
                                stdout);
                  std::printf ("%*s",
                               owner_width + unicode::padding_offset (f.owner),
                               f.owner.c_str ());
                  if (Arguments::color)
                    std::fputs ("\x1b[0m", stdout);
                }
                break;

              case LongColumn::group_name:
                {
                  if (Arguments::color)
                    std::fputs (f.group == "?"sv
                                  ? error_color
                                  : name_color,
                                stdout);
                  std::printf ("%*s",
                               group_width + unicode::padding_offset (f.group),
                               f.group.c_str ());
                  if (Arguments::color)
                    std::fputs ("\x1b[0m", stdout);
                }
                break;

              case LongColumn::size:
                {
                  if (f.type == FileType::directory)
                    {
                      if (Arguments::color)
                        std::printf ("%s%*s\x1b[0m", dir_size_color,
                                     size_width, "<DIR>");
                      else
                        std::printf ("%*s", size_width, "<DIR>");
                    }
                  else
                    print_size (f.size, size_width);
                }
                break;

              case LongColumn::date:
                {
                  if (invalid_time || !f.time)
                    {
                      std::printf ("            %s?\x1b[0m", error_color, stdout);
                      invalid_time = false;
                    }
                  else
                    {
                      std::memset (date_buf, 0, date_sz);
                      let const t = std::localtime (&f.time);
                      if (difftime (f.time, G_six_months_ago) < 0)
                        std::strftime (date_buf, date_sz, "%d. %b  %Y", t);
                      else
                        std::strftime (date_buf, date_sz, "%d. %b %H:%M", t);
                      std::fputs (date_buf, stdout);
                    }
                }
                break;

              case LongColumn::name:
                {
                  print_file_name (f, name_width);
                }
                break;

              case LongColumn::text:
                {
                  let const text = col.get_text ();
                  if (text.size () == 1)
                    std::putchar (text.front ());
                  else
                    std::printf ("%.*s", static_cast<int> (text.size ()),
                                 text.data ());
                }
                break;
            }
        }
      std::putchar ('\n');
    }
}

def print_columns (const FileList &files) -> void
{
  Columns cols;
  for (let const &f : files)
    cols.add (&f);
  cols.print ();
}