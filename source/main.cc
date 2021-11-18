#include "stdafx.hh"
#include "lst.hh"

def main (const int argc, const char *argv[]) -> int
{
#ifdef _WIN32
  SetConsoleOutputCP (CP_UTF8);

  // ONLY FOR POWERSHELL:
  // Windows gives us the full path of the executable which looks quite ugly in
  // error messages so let's just use the file name.
  // Sadly there is no way of checking if the program was launched by using
  // just it's name or the full path.
  //
  // We do this for CMD as well anyways since checking which one was used to
  // invoke the program takes too much processing effort.
  // (who uses full paths on Windows anyways)
  let const last_slash = std::string_view (argv[0]).rfind ('\\') + 1;
  argv[0] += last_slash;
#endif

  G_is_a_tty = isatty (fileno (stdout));

  arena::vector<fs::path> args;
  args.reserve (argc);
  if (!parse_args (argc, argv, args))
    {
      std::fprintf (stderr, "Try '%s --help' for more information\n", argv[0]);
      return 1;
    }

  if (Arguments::long_listing)
    {
      // Get time 6 months ago to choose between time formats
      std::time_t now = std::time (NULL);
      std::tm *six_months_ago = localtime (&now);
      if (six_months_ago->tm_mon < 6)
        {
          six_months_ago->tm_mon = 11 - six_months_ago->tm_mon;
          --six_months_ago->tm_year;
        }
      else
        six_months_ago->tm_mon -= 6;
      G_six_months_ago = mktime (six_months_ago);

#ifdef _WIN32
      // Get interfaces to resolve shortcut targets
      if (FAILED (CoInitialize (nullptr)))
        {
          G_has_shortcut_interfaces = false;
          std::fprintf (stderr, "%s: Failed to initialize the COM library\n",
                        G_program);
        }
      else
        {
          if (FAILED (CoCreateInstance (CLSID_ShellLink, NULL,
                                        CLSCTX_INPROC_SERVER, IID_IShellLink,
                                        reinterpret_cast<void **> (&G_sl))))
            {
              G_has_shortcut_interfaces = false;
              std::fprintf (stderr, "%s: Failed to get IShellLink interface\n",
                            G_program);
            }

          if (FAILED (G_sl->QueryInterface(IID_IPersistFile,
                                           reinterpret_cast<void **> (&G_pf))))
            {
              G_has_shortcut_interfaces = false;
              std::fprintf (stderr, "%s: Failed to get IPersistFile interface\n",
                            G_program);
            }
        }
#endif
    }

  if (!Arguments::width)
    {
      if (G_is_a_tty)
        {
          // Get terminal width
#ifdef _WIN32
          CONSOLE_SCREEN_BUFFER_INFO csbi;
          GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);
          Arguments::width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
          struct winsize ws;
          ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws);
          Arguments::width = ws.ws_col;
#endif
        }
      else
        Arguments::width = 80;
    }

  if (Arguments::long_columns.empty ())
    {
      parse_long_format (default_long_output_format, Arguments::long_columns);
    }

  if (args.empty ())
    args.emplace_back (".");

  let need_label = false;

  for (let &a : args)
    {
      a.make_preferred ();
      if (!fs::exists (a))
        {
          std::printf ("%s: '%s': No such file or directory\n", G_program,
                       unicode::path_to_str (a).c_str ());
          need_label = true;
          continue;
        }
      else if (!Arguments::immediate_dirs && fs::is_directory (a))
        list_dir (a);
      else
        list_file (a);
    }

  void (*print_files) (const FileList &files) =
    (Arguments::long_listing
     ? print_long
     : (Arguments::single_column
        ? print_single_column
        : print_columns
    ));

  if (!G_singles.empty ())
    {
      sort_files (G_singles);
      print_files (G_singles);
    }

  need_label = need_label || (!G_singles.empty () || G_directories.size () > 1);
  let sep = !G_singles.empty ();

  for (let &d : G_directories)
    {
      if (sep)
        std::putchar ('\n');
      else
        sep = true;

      if (need_label)
        std::printf ("%s:\n", d.first.string ().c_str ());

      sort_files (d.second);
      print_files (d.second);
    }

  return 0;
}
