#include "lst.hh"
#include "columns.hh"
#include "match.hh"
#include "natural_sort.hh"

#ifdef _WIN32
static const fs::path S_lnk_ext { L".lnk"s };
static const fs::path S_exe_ext { L".exe"s };
static const fs::path S_bat_ext { L".bat"s };
static const fs::path S_cmd_ext { L".cmd"s };
static const fs::path S_bak_ext { L".bak"s };
static const fs::path S_tmp_ext { L".tmp"s };
#else
static const fs::path S_bak_ext { ".bak"s };
static const fs::path S_tmp_ext { ".tmp"s };
#endif

std::time_t G_six_months_ago;

FileList G_singles;
std::list<std::pair<fs::path, FileList>> G_directories;

static std::error_code S_ec;
static bool S_did_complain;

#ifdef _WIN32
static std::map<PSID, arena::string> G_users;
static std::map<PSID, arena::string> G_groups;
#else
std::map<pid_t, arena::string> G_users;
std::map<uid_t, arena::string> G_groups;
#endif


static def complain (const fs::path &file)
{
  if (!S_did_complain)
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
      S_did_complain = true;
    }
  S_ec.clear ();
}


static def escape_nongraphic (char32_t c, arena::string &out)
{
  static constexpr char hex_digits[] = "0123456789ABCDEF";
  out.push_back ('\\');
  if constexpr (ascii_escape_octal)
    {
      out.push_back ('0' + c / 64);
      out.push_back ('0' + (c / 8) % 8);
      out.push_back ('0' + c % 8);
    }
  else
    {
      out.push_back ('x');
      out.push_back (hex_digits[c / 16]);
      out.push_back (hex_digits[c % 16]);
    }
}


static def add_frills (const arena::string &str, arena::string &out)
{
  let const always_quote = Arguments::quoting == QuoteMode::double_;
  let need_quoting = false;
  let quote_char = always_quote ? '"' :  U'\0';
  let p = str.c_str ();
  char32_t c;
  int cp_size = 0;

  out.reserve (str.size () * 1.2);

  // Source: https://www.opencoverage.net/coreutils/index_html/source_213.html
  // Quote the name if it starts with any of there
  let constexpr quote_if_first = "#~"sv;
  // Quote the name if it contains any of these
  let constexpr quote_if_anywhere = " !$&()*;<=>[^`|"sv;

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
    }
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
        {
          out.push_back ('\\');
          out.push_back (c);
        }
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
  : _path (fs::absolute (p))
{
  let const p_str = unicode::path_to_str (p);
  add_frills (p_str, name);

  let const ext = p.extension ();

  if (ext == S_tmp_ext || ext == S_bak_ext || p_str.back () == '~')
    is_temporary = true;

#ifdef _WIN32
  if (ext == S_exe_ext || ext == S_bat_ext || ext == S_cmd_ext)
    is_executable = true;
#else
  if (s.type () == fs::file_type::regular)
    {
      let const perms = s.permissions ();
      if ((perms & fs::perms::owner_exec) != fs::perms::none
          || (perms & fs::perms::group_exec) != fs::perms::none
          || (perms & fs::perms::others_exec) != fs::perms::none)
        is_executable = true;
    }
#endif

#ifdef _WIN32
  if (target || ext == S_lnk_ext)
    type = fs::file_type::symlink;
  else
#endif
  type = s.type ();
}


FileInfo::FileInfo (const fs::path &p, const fs::file_status &in_s)
  : _path (fs::absolute (p))
{
  let const status = [](const fs::path &p) {
    return Arguments::dereference ? fs::status (p, S_ec) : fs::symlink_status (p, S_ec);
  };
  let const s = status (p);
  S_did_complain = false;
  if (S_ec)
    {
      complain (p);
      add_frills (unicode::path_to_str (p.filename ()), name);
      status_failed = true;
      return;
    }

  let p_str = unicode::path_to_str (p.filename ());
  add_frills (p_str, name);
  let ext = p.extension ();

#ifdef _WIN32
  HANDLE handle;
  static BY_HANDLE_FILE_INFORMATION file_info;
  let const flags = (FILE_FLAG_BACKUP_SEMANTICS
                     | (Arguments::dereference ? 0 : FILE_FLAG_OPEN_REPARSE_POINT));
  handle = CreateFileW (fs::absolute (p).wstring ().c_str (),
                        GENERIC_READ,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr,
                        OPEN_EXISTING,
                        flags,
                        nullptr);
  if (handle == INVALID_HANDLE_VALUE
      || !GetFileInformationByHandle (handle, &file_info))
    {
      complain (p);
      return;
    }
#else
  static struct stat sb;
  if ((Arguments::dereference ? stat : lstat)
      (fs::absolute (p).string<char> (arena::Allocator<char> {}).c_str (), &sb) == -1)
    {
      complain (p);
      return;
    }
  // Alias so we can just use handle and file_info
  struct stat *const handle = &sb;
  struct stat &file_info = sb;
#endif

  if (Arguments::long_listing)
    {
      if (!get_owner_and_group (handle, owner, group))
        {
#ifdef _WIN32
          S_ec = std::error_code (GetLastError (), std::system_category ());
#else
          S_ec = std::error_code (errno, std::system_category ());
#endif
          complain (p);
        }

      size = fs::is_directory (s) ? 0 : get_file_size (&file_info);

      if (!get_file_time (handle, time))
        {
#ifdef _WIN32
          S_ec = std::error_code (GetLastError (), std::system_category ());
#else
          S_ec = std::error_code (errno, std::system_category ());
#endif
          complain (p);
          time = 0;
        }

      link_count = get_link_count (&file_info);

      perms = s.permissions ();

      if (s.type () == fs::file_type::symlink)
        {
          let const real_tp = fs::read_symlink (p);
          let const tp = (real_tp.is_absolute ()
                          ? real_tp
                          : fs::absolute (p.parent_path() / real_tp));
          target = new FileInfo (real_tp, status (tp), link_target_tag {});
        }
#ifdef _WIN32
      else if (G_has_shortcut_interfaces && ext == S_lnk_ext)
        {
          arena::string target_name;
          get_shortcut_target (p, target_name);
          let const tp = fs::path (target_name);
          target = new FileInfo (tp, status (tp), link_target_tag {});
        }
#endif
    }
  else
    {
      // Only get extra information needed for sorting
      switch (Arguments::sort_mode)
        {
          case SortMode::size:
            size = fs::is_directory (s) ? 0 : get_file_size (&file_info);
            break;
          case SortMode::time:
            get_file_time (handle, time);
            break;
          // All other sorting modes just need the name
          default:;
        }
    }

  // Get the correct name for name dependant file types
  if (Arguments::dereference && in_s.type () == fs::file_type::symlink)
    {
      let const real_tp = fs::read_symlink (p);
      let const tp = (real_tp.is_absolute ()
                      ? real_tp
                      : fs::absolute (p.parent_path() / real_tp));
      ext = tp.extension ();
      p_str = unicode::path_to_str (tp);
    }

  if (ext == S_tmp_ext || ext == S_bak_ext || p_str.back () == '~')
    is_temporary = true;

#ifdef _WIN32
  if (ext == S_exe_ext || ext == S_bat_ext || ext == S_cmd_ext)
    is_executable = true;
#else
  if (s.type () == fs::file_type::regular)
    {
      let const perms = s.permissions ();
      if ((perms & fs::perms::owner_exec) != fs::perms::none
          || (perms & fs::perms::group_exec) != fs::perms::none
          || (perms & fs::perms::others_exec) != fs::perms::none)
        is_executable = true;
    }
#endif

#ifdef _WIN32
  if (target || ext == S_lnk_ext)
    type = fs::file_type::symlink;
  else
#endif
  type = s.type ();
}


FileInfo::~FileInfo ()
{
  delete target;
}


def list_file (const fs::path &path) -> void
{
  G_singles.emplace_back (path, fs::symlink_status (path));
}


def list_dir (const fs::path &path) -> void
{
  FileList *l = &G_directories.emplace_back (std::make_pair (path, FileList {})).second;

  for (let e : fs::directory_iterator (path))
    {
      let const pstr = unicode::path_to_str (e.path ().filename ());
      if (!Arguments::all && pstr[0] == '.')
        continue;
      if (Arguments::ignore_backups
          && (pstr.ends_with (".tmp"sv)
              || pstr.ends_with (".bak"sv)
              || pstr.back () == '~'))
        continue;
      for (let pattern : Arguments::ignore_patterns)
        {
          if (match (pattern, pstr))
            goto skip_file;
        }

      l->emplace_back (e.path (), e.symlink_status ());

      if (Arguments::recursive && e.is_directory ())
        list_dir (e.path ());
skip_file:;
    }
}

#ifdef _WIN32

IShellLink *G_sl = nullptr;
IPersistFile *G_pf = nullptr;
bool G_has_shortcut_interfaces = true;

def get_owner_and_group (HANDLE file_handle, arena::string &owner_out,
                         arena::string &group_out) -> bool
{
  PSID owner_sid;
  PSECURITY_DESCRIPTOR sd;
  SID_NAME_USE use = SidTypeUnknown;
  DWORD owner_size = 0, group_size = 0;

  if (GetSecurityInfo (file_handle, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                       &owner_sid, NULL, NULL, NULL, &sd)
      != ERROR_SUCCESS)
    {
      owner_out.assign ("?");
      group_out.assign ("?");
      return false;
    }

  if (G_users.contains (owner_sid))
    {
      owner_out.assign (G_users[owner_sid]);
      group_out.assign (G_groups[owner_sid]);
    }
  else
    {
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

      G_users[owner_sid] = owner_out;
      G_groups[owner_sid] = group_out;
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

static def win_file_time_to_time_t (FILETIME ft) -> std::time_t
{
  ULARGE_INTEGER conv;
  conv.LowPart = ft.dwLowDateTime;
  conv.HighPart = ft.dwHighDateTime;
  return conv.QuadPart / 10000000ULL - 11644473600ULL;
}

def get_file_time (HANDLE file_handle, std::time_t &out) -> bool
{
  FILETIME access, write, creation;

  if (!GetFileTime (file_handle, &creation, &access, &write))
    {
      out = 0;
      return false;
    }

  switch (Arguments::time_mode)
    {
      case TimeMode::access: out = win_file_time_to_time_t (access); break;
      case TimeMode::write: out = win_file_time_to_time_t (write); break;
      case TimeMode::creation: out = win_file_time_to_time_t (creation); break;
    }

  return true;
}

def get_file_size (BY_HANDLE_FILE_INFORMATION *file_info) -> std::uintmax_t
{
  return ((static_cast<std::uintmax_t> (file_info->nFileSizeHigh) << sizeof (DWORD))
          | file_info->nFileSizeLow);
}

def get_link_count (BY_HANDLE_FILE_INFORMATION *file_info) -> unsigned
{
  return file_info->nNumberOfLinks;
}

#else // _WIN32

def get_owner_and_group (struct stat *sb, arena::string &owner_out,
                         arena::string &group_out) -> bool
{
  if (!G_users.contains (sb->st_uid))
    G_users[sb->st_uid] = arena::string (getpwuid (sb->st_uid)->pw_name);
  owner_out.assign (G_users[sb->st_uid]);

  if (!G_groups.contains (sb->st_gid))
    G_groups[sb->st_gid] = arena::string (getgrgid (sb->st_gid)->gr_name);
  group_out.assign (G_groups[sb->st_gid]);
  return true;
}

def get_file_time (struct stat *sb, std::time_t &out) -> bool
{
  switch (Arguments::time_mode)
    {
      case TimeMode::access: out = sb->st_atime; break;
      case TimeMode::write: out = sb->st_mtime; break;
      // st_ctime is not the creation time but the time of the last inode
      // change. Unix, per standard, does not store the creation time so this
      // is the best we can get.
      case TimeMode::creation: out = sb->st_ctime; break;
    }

  return true;
}

def get_file_size (struct stat *sb) -> std::uintmax_t
{
  return static_cast<std::uintmax_t> (sb->st_size);
}

def get_link_count (struct stat *sb) -> unsigned
{
  return static_cast<unsigned> (sb->st_nlink);
}

#endif // _WIN32


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

  let compare_path = [](const fs::path &a, const fs::path &b) -> int {
    if (Arguments::case_sensitive)
      return a.compare (b);
    else
      return case_insensitive_compare (a, b);
  };

  #define SORT_FUNC [&compare_path](const FileInfo &a, const FileInfo &b)
  if (!sort)
    {
      switch (Arguments::sort_mode)
        {
          break; case SortMode::name:
            sort = SORT_FUNC {
              return (compare_path (a._path, b._path) < 0) ^ Arguments::reverse;
            };
          break; case SortMode::extension:
            sort = SORT_FUNC {
              let const c = compare_path (a._path.extension (),
                                          b._path.extension ());
              // If both extensions are equal, compare the entire filename
              return (((c ? c : compare_path (a._path, b._path)) < 0)
                      ^ Arguments::reverse);
            };
          break; case SortMode::size:
            sort = SORT_FUNC {
              // If both sizes are equal, compare the filename
              return ((a.size == b.size
                       ? compare_path (a._path, b._path) < 0
                       : a.size > b.size)
                      ^ Arguments::reverse);
            };
          break; case SortMode::time:
            sort = SORT_FUNC {
              let const d = difftime (a.time, b.time);
              // If both times are equal, compare the file name
              return ((d ? d > 0 : compare_path (a._path, b._path) < 0)
                      ^ Arguments::reverse);
            };
          break; case SortMode::version:
            sort = SORT_FUNC {
              (void)compare_path;  // Suppress unused capture warning
              return ((natural_compare (a._path.filename (),
                                       b._path.filename ()) < 0)
                      ^ Arguments::reverse);
            };
          break; case SortMode::name_length:
            sort = SORT_FUNC {
              let const alen = unicode::display_width (unicode::path_to_str (a._path));
              let const blen = unicode::display_width (unicode::path_to_str (b._path));
              return (alen == blen
                      ? compare_path (a._path, b._path) < 0
                      : alen < blen);
            };
          break; case SortMode::none:;
        }
    }
  #undef SORT_FUNC

  if (Arguments::sort_mode != SortMode::none)
    files.sort (sort);

  if (Arguments::group_directories_first)
    files.sort ([](const FileInfo &a, const FileInfo &b) {
      return (((a.type == fs::file_type::directory)
               > (b.type == fs::file_type::directory))
              ^ Arguments::reverse);
    });
}


static def file_color (const FileInfo &f) -> const char *
{
  if (f.status_failed)
    return file_name_error_color;
  else if (f.is_executable)
    return "\x1b[92m";
  else if (f.is_temporary)
    return "\x1b[90m";

  switch (f.type)
    {
      case fs::file_type::directory:  return "\x1b[94m";
      case fs::file_type::symlink:    return "\x1b[96m";
      case fs::file_type::unknown:    return file_name_error_color;
      default:                   return "";
    }
}


def file_indicator (const FileInfo &f) -> char
{
  if (!Arguments::file_type && f.is_executable)
    return '*';

  switch (f.type)
    {
      case fs::file_type::directory:  return '/';
      case fs::file_type::symlink:    return '@';
      case fs::file_type::fifo:       return '|';
      case fs::file_type::socket:     return '=';
      case fs::file_type::not_found:  return '?';
      default:                   return 0;
    }
}


static def file_indicator_color (const FileInfo &f) -> const char *
{
  if (f.is_executable)
    return "\x1b[32m";

  switch (f.type)
    {
      case fs::file_type::directory:  return "\x1b[34m";
      case fs::file_type::symlink:    return "\x1b[36m";
      case fs::file_type::fifo:       return "\x1b[90m";
      case fs::file_type::socket:     return "\x1b[90m";
      case fs::file_type::not_found:  return "\x1b[91m";
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
  if (Arguments::hyperlinks)
    std::printf ("\x1b]8;;file:///%s\x1b\\%s\x1b]8;;\x1b\\",
                 unicode::path_to_str (f._path).c_str (),
                 f.name.c_str ());
  else
    std::fputs (f.name.c_str (), stdout);
  if (Arguments::color)
    std::fputs ("\x1b[0m", stdout);
  // Indicator
  if (Arguments::classify
      && !(Arguments::long_listing && f.type == fs::file_type::symlink && f.target))
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
  if (Arguments::human_readble && size >= Arguments::human_readble)
    {
      let fsize = double (size);
      let p = 0;
      while (fsize >= Arguments::human_readble)
        {
          ++p;
          fsize /= Arguments::human_readble;
        }

      let unit = (Arguments::human_readble == 1000 ? units_1000 : units_1024)[p - 1];
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
      if constexpr (dry)
        return std::snprintf (nullptr, 0, "%ju", size);
      else
        {
          if (Arguments::color)
            return std::printf ("%s%*ju\x1b[0m", file_size_color, width, size);
          else
            return std::printf ("%*ju", width, size);
        }
    }
}


static def strftime_width (const std::tm *time) -> int
{
  static arena::string S_buf;
  static bool first_call = true;
  if (first_call)
    {
      S_buf.resize (64);
      first_call = false;
    }

  let r = std::strftime (S_buf.data (), S_buf.size () - 1, Arguments::time_format, time);

  while (!r)
    {
      S_buf.resize (S_buf.size () << 1);
      r = std::strftime (S_buf.data (), S_buf.size () - 1, Arguments::time_format, time);
    }

  S_buf.resize (r);
  return unicode::display_width (S_buf);
}


def print_single_column (const FileList &files) -> void
{
  let has_quoted = false;
  for (let const &f : files)
    {
      if ((Arguments::quoting == QuoteMode::default_)
           && (f.name.front () == '\'' || f.name.front () == '"')
           && (f.name.front () == f.name.back ()))
        {
          has_quoted = true;
          break;
        }
    }

  for (let const &f : files)
    {
      if (has_quoted && !(f.name.front () == '\'' || f.name.front () == '"'))
        std::putchar (' ');
      print_file_name (f);
      std::putchar ('\n');
    }
}


def print_long (const FileList &files) -> void
{
  arena::string size_buf;
  //const int date_sz = 14;
  //char date_buf[date_sz];
  bool invalid_time = false;

  int name_width = 0, link_width = 0, owner_width = 0, group_width = 0,
      size_width = 0, time_width = Arguments::time_format ? 0 : 13;

  let const name_is_last = Arguments::long_columns.back () == LongColumn::name;

  // Get column widths
  for (let const &f : files)
    {
      if (Arguments::long_columns_has.test (LongColumn::name) && !name_is_last)
        name_width = std::max (name_width, file_name_width (f));
      if (Arguments::long_columns_has.test (LongColumn::hard_link_count))
        link_width = std::max (link_width, int_len (f.link_count));
      if (Arguments::long_columns_has.test (LongColumn::owner_name))
        owner_width = std::max (owner_width, unicode::display_width (f.owner));
      if (Arguments::long_columns_has.test (LongColumn::group_name))
        group_width = std::max (group_width, unicode::display_width (f.group));
      if (Arguments::long_columns_has.test (LongColumn::size))
        {
          if (f.type == fs::file_type::directory)
            size_width = std::max (size_width, 5);
          else
            size_width = std::max (size_width, print_size<true> (f.size));
        }
      if (Arguments::time_format && !f.status_failed && f.time)
        {
          let const t = std::localtime (&f.time);
          time_width = std::max (time_width, strftime_width (t));
        }
    }

  let const date_sz = time_width + 1;
  static arena::string date_buf_s;
  date_buf_s.resize (date_sz);
  let const date_buf = date_buf_s.data ();

  for (let const &f : files)
    {
      if (f.status_failed)
        invalid_time = true;

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
                  if (f.type == fs::file_type::directory)
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
                      std::printf ("%s%*c\x1b[0m", error_color, time_width, '?');
                      invalid_time = false;
                    }
                  else
                    {
                      std::memset (date_buf, 0, date_sz);
                      let const t = std::localtime (&f.time);
                      if (Arguments::time_format)
                        {
                          std::strftime (date_buf, date_sz, Arguments::time_format, t);
                        }
                      else
                        {
                          if (difftime (f.time, G_six_months_ago) < 0)
                            std::strftime (date_buf, date_sz, "%d. %b  %Y", t);
                          else
                            std::strftime (date_buf, date_sz, "%d. %b %H:%M", t);
                      }
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
  if (files.empty ())
    return;
  Columns cols;
  for (let const &f : files)
    cols.add (&f);
  cols.print ();
}
