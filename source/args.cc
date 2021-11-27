#include "args.hh"
#include "options.hh"

bool G_is_a_tty;

namespace Arguments
{
bool all = false;
bool long_listing = false;
bool single_column = false;
bool color = true;
bool recursive = false;
bool classify = true;
bool file_type = false;
bool immediate_dirs = false;
bool reverse = false;
SortMode sort_mode = SortMode::name;
QuoteMode quoting = QuoteMode::default_;
NongraphicMode nongraphic = NongraphicMode::escape;
unsigned human_readble = 0;
unsigned width = 0;
bool english_errors = false;
bool group_directories_first = true;
arena::vector<LongColumn> long_columns {};
std::bitset<LongColumn::text + 1> long_columns_has {};
bool case_sensitive = false;
bool ignore_backups = false;
bool dereference = false;
TimeMode time_mode = TimeMode::write;
const char *time_format = nullptr;
bool hyperlinks = false;
arena::vector<std::string_view> ignore_patterns;
}

const char *G_program;

static def usage ()
{
  std::puts ("Usage: lst [OPTION]... [FILE]...");
  std::puts ("List information about the FILEs (the current directory by default).");

  std::puts ("\nOptions:");
  std::puts ("  -a, --all             Do not ignore entries starting with '.'");
  std::puts ("  -b, --escape          Print C-style escapes for nongraphic characters.");
  std::puts ("  -B, --ignore-backups  Do not list entries ending with '~', '.bak', or '.tmp'");
  std::puts ("  -c                    Use creation time for time.");
  std::puts ("  -d, --directory       Show directory names instead of contents.");
  std::puts ("  -D,                   Do not group directories before files.");
  std::puts ("  -F, --no-classify     Do not append indicator to entries.");
  std::puts ("      --file-type       Do not append '*' indicator.");
  std::puts ("      --format=FORMAT   Format string for long listing format");
  std::puts ("                          '$t': Type indicator");
  std::puts ("                          '$p': rwx style permissions");
  std::puts ("                          '$P': octal permissions");
  std::puts ("                          '$l': Link count");
  std::puts ("                          '$o': Owner");
  std::puts ("                          '$g': Group");
  std::puts ("                          '$s': Size");
  std::puts ("                          '$d': Datetime");
  std::puts ("                          '$n': File name (with -l, also the link target)");
  std::puts ("                         Anything else is printed literally;");
  std::printf ("                         the default value is '%.*s'.\n",
               static_cast<int> (default_long_output_format.size ()),
               default_long_output_format.data ());
  std::puts ("  -h, --human-readable  Print sizes like 1K 234M 2G etc.");
  std::puts ("      --hyperlinks      Hyperlink file names.");
  std::puts ("      --si              Like -h, but use powers of 1000 not 1024.");
  std::puts ("      --ignore=PATTERN  Do not list entries matching shell PATTERN.");
  std::puts ("  -l                    Use long listing format.");
  std::puts ("  -L, --dereference     When showing file information about a symbolic link,");
  std::puts ("                          show information for the file the link references");
  std::puts ("                          rather than for the link itself.");
  std::puts ("  -N, --literal         Do not quote file names.");
  std::puts ("  -q, --hide-control-chars");
  std::puts ("                        Print '?' instead of nongraphic characters.");
  std::puts ("      --show-control-chars");
  std::puts ("                        Show nongraphic characters as-is.");
  std::puts ("  -Q, --quote-name      Enclose entry names in double quotes.");
  std::puts ("  -r, --reverse         Reverse sorting.");
  std::puts ("  -R, --recursive       List subdirectories recursively.");
  std::puts ("  -S                    Sort by file size, largest first.");
  std::puts ("      --sort=WORD       Sort by WORD instead of name: none (-U), time (-t),");
  std::puts ("                          size (-S), extension (-X), version (-v).");
  std::puts ("      --case-sensitive  Do not ignore case when sorting by name or extension.");
  std::puts ("  -t                    Sort y time, newest first.");
  std::puts ("      --time=WORD       Change the default of using modification times;");
  std::puts ("                          creation time (-c): creation, birth");
  std::puts ("                          access time (-u): atime, access, use;");
  std::puts ("                          change time: ctime, write;");
  std::puts ("                         with -l, WORD determines which time is shown;");
  std::puts ("                         with --sort=time, sort by WORD (newest first).");
  std::puts ("      --time-format=FORMAT");
  std::puts ("                        Specify custom time/date format; man strftime.");
  std::puts ("  -u                    Use time of last access for time.");
  std::puts ("  -U                    Do not sort; list entries in directory order.");
  std::puts ("  -v                    Natural sort of version numbers within file names.");
  std::puts ("      --width=COLS      Set the output width for multi column output to COLS.");
  std::puts ("  -X                    Sort alphabetically by entry extension.");
  std::puts ("  -1                    List one file per line.");
  std::puts ("      --english-errors  For Windows, print filesystem related error messages");
  std::puts ("                          in english instead of the current display language.");
}

static inline def handle_short_opt (char flag)
{
  switch (flag)
    {
      case 'a': Arguments::all = true; break;
      case 'l': Arguments::long_listing = true; break;
      case '1': Arguments::single_column = true; break;
      case 'R': Arguments::recursive = true; break;
      case 'F': Arguments::classify = false; break;
      case 'v': Arguments::sort_mode = SortMode::version; break;
      case 'S': Arguments::sort_mode = SortMode::size; break;
      case 'X': Arguments::sort_mode = SortMode::extension; break;
      case 't': Arguments::sort_mode = SortMode::time; break;
      case 'U': Arguments::sort_mode = SortMode::none; break;
      case 'N': Arguments::quoting = QuoteMode::literal; break;
      case 'Q': Arguments::quoting = QuoteMode::double_; break;
      case 'b': Arguments::nongraphic = NongraphicMode::escape; break;
      case 'q': Arguments::nongraphic = NongraphicMode::hide; break;
      case 'h': Arguments::human_readble = 1024; break;
      case 'd': Arguments::immediate_dirs = true; break;
      case 'r': Arguments::reverse = true; break;
      case 'D': Arguments::group_directories_first = false; break;  // Maybe use 'G' instead
      case 'B': Arguments::ignore_backups = true; break;
      case 'L': Arguments::dereference = true; break;
      case 'u': Arguments::time_mode = TimeMode::access; break;
      case 'c': Arguments::time_mode = TimeMode::creation; break;
      default:
        std::fprintf (stderr, "%s: invalid option -- %c\n", G_program, flag);
        return false;
    }
  return true;
}

static inline def handle_long_opt (std::string_view elem) -> bool
{
  let const eq_pos = elem.find ('=');
  let const has_arg = eq_pos != decltype (elem)::npos;
  let const opt_name = has_arg ? elem.substr (2, eq_pos - 2) : elem.substr (2);
  let const arg = has_arg ? elem.substr (eq_pos + 1) : ""sv;

  let const require_arg = [&arg, &opt_name]() {
    if (arg.empty ())
      {
        std::fprintf (stderr, "%s: option ‘--%.*s’ requires and argument\n",
                      G_program,
                      static_cast<int> (opt_name.size ()), opt_name.data ());
        return true;
      }
    return false;
  };

  using namespace Arguments;

  if (opt_name == "help")
    {
      usage ();
      exit (0);
    }
  else if (opt_name ==                     "all"sv) all = true;
  else if (opt_name ==               "recursive"sv) recursive = true;
  else if (opt_name ==             "no-classify"sv) classify = false;
  else if (opt_name ==               "file-type"sv) file_type = true;
  else if (opt_name ==                 "literal"sv) quoting = QuoteMode::literal;
  else if (opt_name ==              "quote-name"sv) quoting = QuoteMode::double_;
  else if (opt_name ==                  "escape"sv) nongraphic = NongraphicMode::escape;
  else if (opt_name ==      "hide-control-chars"sv) nongraphic = NongraphicMode::hide;
  else if (opt_name ==      "show-control-chars"sv) nongraphic = NongraphicMode::show;
  else if (opt_name ==          "human-readable"sv) human_readble = 1024;
  else if (opt_name ==                      "si"sv) human_readble = 1000;
  else if (opt_name ==               "directory"sv) immediate_dirs = true;
  else if (opt_name ==                 "reverse"sv) reverse = true;
  else if (opt_name ==          "english-errors"sv) english_errors = true;
  else if (opt_name ==          "case-sensitive"sv) case_sensitive = true;
  else if (opt_name ==          "ignore-backups"sv) ignore_backups = true;
  else if (opt_name ==             "dereference"sv) dereference = true;
  else if (opt_name ==               "hyperlink"sv) hyperlinks = true;

  else if (opt_name == "color"sv)
    {
      if (arg.empty () || arg == "always"sv || arg == "yes"sv)
        Arguments::color = true;
      else if (arg == "never"sv || arg == "no"sv)
        Arguments::color = false;
      else if (arg == "auto"sv || arg == "tty"sv)
        Arguments::color = G_is_a_tty;
      else
        {
          std::fprintf (stderr, "%s: invalid argument ‘%.*s’ for ‘--%.*s’\n",
                        G_program,
                        static_cast<int> (arg.size ()), arg.data (),
                        static_cast<int> (opt_name.size ()), opt_name.data ());
          std::fputs ("Valid arguments are:\n", stderr);
          std::fputs ("  - ‘always’, ‘yes’\n", stderr);
          std::fputs ("  - ‘never’, ‘no’\n", stderr);
          std::fputs ("  - ‘auto’, ‘tty’\n", stderr);
          return false;
        }
    }
  else if (opt_name == "sort"sv)
    {
      if (require_arg ()) return false;
      if (arg == "none"sv)
        Arguments::sort_mode = SortMode::none;
      else if (arg == "extension"sv) Arguments::sort_mode = SortMode::extension;
      else if (arg ==      "size"sv) Arguments::sort_mode = SortMode::size;
      else if (arg ==      "time"sv) Arguments::sort_mode = SortMode::time;
      else if (arg ==   "version"sv) Arguments::sort_mode = SortMode::version;
      else
        {
          std::fprintf (stderr, "%s: invalid argument ‘%.*s’ for ‘--%.*s’\n",
                        G_program,
                        static_cast<int> (arg.size ()), arg.data (),
                        static_cast<int> (opt_name.size ()), opt_name.data ());
          std::fputs ("Valid arguments are:\n", stderr);
          std::fputs ("  - ‘none’\n", stderr);
          std::fputs ("  - ‘extension’\n", stderr);
          std::fputs ("  - ‘size’\n", stderr);
          std::fputs ("  - ‘time’\n", stderr);
          std::fputs ("  - ‘version’\n", stderr);
          return false;
        }
    }
  else if (opt_name == "width"sv)
    {
      if (require_arg ()) return false;
      char *end = nullptr;
      Arguments::width = std::strtoul (arg.data (), &end, 0);

      if (Arguments::width == 0 || end != arg.data () + arg.size ())
        {
          std::fprintf (stderr, "%s: invalid argument ‘%.*s’ for ‘--%.*s’\n",
                        G_program,
                        static_cast<int> (arg.size ()), arg.data (),
                        static_cast<int> (opt_name.size ()), opt_name.data ());
          std::fputs ("Argument must be an integer that is greater than zero\n",
                      stderr);
          return false;
        }
    }
  else if (opt_name == "format"sv)
    {
      if (require_arg ()) return false;
      if (!parse_long_format (arg, Arguments::long_columns))
        return false;
    }
  else if (opt_name == "time"sv)
    {
      if (require_arg ()) return false;
      if (arg == "atime"sv || arg == "access"sv || arg == "use"sv)
        Arguments::time_mode = TimeMode::access;
      else if (arg == "ctime"sv || arg == "write"sv)
        Arguments::time_mode = TimeMode::write;
      else if (arg == "creation"sv || arg == "birth"sv)
        Arguments::time_mode = TimeMode::creation;
      else
        {
          std::fprintf (stderr, "%s: invalid argument ‘%.*s’ for ‘--%.*s’\n",
                        G_program,
                        static_cast<int> (arg.size ()), arg.data (),
                        static_cast<int> (opt_name.size ()), opt_name.data ());
          std::fputs ("Valid arguments are:\n", stderr);
          std::fputs ("  - ‘atime’, ‘access’, ‘use’\n", stderr);
          std::fputs ("  - ‘ctime’, ‘write’\n", stderr);
          std::fputs ("  - ‘creation’, ‘birth’\n", stderr);
        }
    }
  else if (opt_name == "time-format"sv)
    {
      if (require_arg ()) return false;
      Arguments::time_format = arg.data ();
    }
  else if (opt_name == "ignore"sv)
    {
      if (require_arg ()) return false;
      Arguments::ignore_patterns.push_back (arg);
    }
  else
    {
      std::fprintf (stderr, "%s: unrecognized option ‘--%.*s’\n", G_program,
                    static_cast<int> (opt_name.size ()), opt_name.data ());
      return false;
    }

  return true;
}

def parse_args (int argc, const char **argv,
                arena::vector<fs::path> &args) -> bool
{
  std::string_view elem;
  int i;
  G_program = argv[0];

  for (i = 1; i < argc; ++i)
    {
      elem = argv[i];
      if (elem[0] == '-')
        {
          if (elem[1] == '-')
            {
              if (elem[2] == '\0')
                break;
              if (!handle_long_opt (elem))
                return false;
            }
          else
            {
              elem.remove_prefix (1);
              for (let f : elem)
                {
                  if (!handle_short_opt (f))
                    return false;
                }
            }
        }
      else
        args.emplace_back (elem);
    }

  for (; i < argc; ++i)
    args.emplace_back (argv[i]);

  return true;
}

def parse_long_format (std::string_view format, arena::vector<LongColumn> &out) -> bool
{
  for (std::size_t i = 0; i < format.size (); ++i)
    {
      if (format[i] == '$')
        {
          let const idx = LongColumn::field_chars.find (format[++i]);
          if (idx == std::string_view::npos)
            {
              std::fprintf (stderr,
                            "%s: invalid format specifier ‘$%c’ for ‘--format’\n",
                            G_program, format[i]);
              std::fputs ("Valid values are:\n", stderr);
              std::fputs ("  - ‘$t’ Type indicator\n", stderr);
              std::fputs ("  - ‘$p’ rwxrwxrwx style permissions\n", stderr);
              std::fputs ("  - ‘$P’ Octal permissions\n", stderr);
              std::fputs ("  - ‘$l’ Hard link count\n", stderr);
              std::fputs ("  - ‘$o’ Owner\n", stderr);
              std::fputs ("  - ‘$g’ Group\n", stderr);
              std::fputs ("  - ‘$s’ Size\n", stderr);
              std::fputs ("  - ‘$d’ Datetime\n", stderr);
              std::fputs ("  - ‘$n’ File name\n", stderr);
              return false;
            }
          if (Arguments::long_columns_has.test (idx))
            {
              std::fprintf (stderr, "%s: duplicate format specifier ‘$%c’\n",
                            G_program, format[i]);
              return false;
            }
          out.emplace_back (static_cast<LongColumn::Enum> (idx));
          Arguments::long_columns_has.set (idx);
        }
      else
        {
          let const end = format.find ('$', i);
          out.emplace_back (format.substr (i, end - i));
          if (end == std::string_view::npos)
            break;
          i = end - 1;
        }
    }
  return true;
}
