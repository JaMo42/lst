#include "args.hh"

bool G_is_a_tty;

namespace Arguments
{
bool all = false;
bool almost_all = false;
bool long_listing = false;
bool single_column = false;
bool color = true;
bool recursive = false;
bool classify = true;
bool immediate_dirs = false;
bool reverse = false;
SortMode sort_mode = SortMode::name;
QuoteMode quoting = QuoteMode::default_;
NongraphicMode nongraphic = NongraphicMode::escape;
unsigned human_readble = 0;
unsigned width = 0;
bool english_errors = false;
bool group_directories_first = false;
std::vector<LongColumn> long_columns {};
bool case_sensitive = false;
}

const char *G_program;

static def usage ()
{
  std::puts ("Usage: lst [OPTION]... [FILE]...");
  std::puts ("List information about the FILEs (the current directory by default).");

  // TODO
}

static inline def handle_short_opt (char flag)
{
  switch (flag)
    {
      case 'a': Arguments::all = true; break;
      case 'A': Arguments::almost_all = true; break;
      case 'l': Arguments::long_listing = true; break;
      case '1': Arguments::single_column = true; break;
      case 'R': Arguments::recursive = true; break;
      case 'F': Arguments::classify = false; break;
      case 'v': Arguments::sort_mode = SortMode::version; break;
      case 'S': Arguments::sort_mode = SortMode::size; break;
      case 'X': Arguments::sort_mode = SortMode::extension; break;
      case 't': Arguments::sort_mode = SortMode::time; break;
      case 'U': Arguments::sort_mode = SortMode::none; break;
      case 'n': Arguments::sort_mode = SortMode::name; break;
      case 'N': Arguments::quoting = QuoteMode::literal; break;
      case 'Q': Arguments::quoting = QuoteMode::double_; break;
      case 'b': Arguments::nongraphic = NongraphicMode::escape; break;
      case 'q': Arguments::nongraphic = NongraphicMode::hide; break;
      case 'h': Arguments::human_readble = 1024; break;
      case 'd': Arguments::immediate_dirs = true; break;
      case 'r': Arguments::reverse = true; break;
      case 'D': Arguments::group_directories_first = true; break;  // Maybe use 'G' instead
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
  else if (opt_name ==                     "all") all = true;
  else if (opt_name ==              "almost-all") almost_all = true;
  else if (opt_name ==               "recursive") recursive = true;
  else if (opt_name ==             "no-classify") classify = false;
  else if (opt_name ==                 "literal") quoting = QuoteMode::literal;
  else if (opt_name ==              "quote-name") quoting = QuoteMode::double_;
  else if (opt_name ==                  "escape") nongraphic = NongraphicMode::escape;
  else if (opt_name ==      "hide-control-chars") nongraphic = NongraphicMode::hide;
  else if (opt_name ==      "show-control-chars") nongraphic = NongraphicMode::show;
  else if (opt_name ==          "human-readable") human_readble = 1024;
  else if (opt_name ==                      "si") human_readble = 1000;
  else if (opt_name ==               "directory") immediate_dirs = true;
  else if (opt_name ==                 "reverse") reverse = true;
  else if (opt_name ==          "english-errors") english_errors = true;
  else if (opt_name == "group-directories-first") group_directories_first = true;
  else if (opt_name ==          "case-sensitive") case_sensitive = true;

  else if (opt_name == "color")
    {
      if (arg.empty () || arg == "always" || arg == "yes")
        Arguments::color = true;
      else if (arg == "never" || arg == "no")
        Arguments::color = false;
      else if (arg == "auto" || arg == "tty")
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
  else if (opt_name == "sort")
    {
      if (require_arg ()) return false;
      if (arg == "none")
        Arguments::sort_mode = SortMode::none;
      else if (arg == "extension") Arguments::sort_mode = SortMode::extension;
      else if (arg ==      "size") Arguments::sort_mode = SortMode::size;
      else if (arg ==      "time") Arguments::sort_mode = SortMode::time;
      else if (arg ==   "version") Arguments::sort_mode = SortMode::version;
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
  else if (opt_name == "width")
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
  else if (opt_name == "format")
    {
      if (require_arg ()) return false;
      if (!parse_long_format (arg, Arguments::long_columns))
        return false;
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
                std::vector<fs::path> &args) -> bool
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

def parse_long_format (std::string_view format, std::vector<LongColumn> &out) -> bool
{
  std::bitset<LongColumn::text + 1> has;
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
              std::fputs ("  - ‘$d’ Last write time\n", stderr);
              std::fputs ("  - ‘$n’ File name\n", stderr);
              return false;
            }
          if (has.test (idx))
            {
              std::fprintf (stderr, "%s: duplicate format specifier ‘$%c’\n",
                            G_program, format[i]);
              return false;
            }
          out.emplace_back (static_cast<LongColumn::Enum> (idx));
          has.set (idx);
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
