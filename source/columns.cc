#include "columns.hh"

unsigned G_term_height;

Columns::Columns ()
  : M_columns {}
  , M_rows (1)
  , M_has_quoted (false)
{}

def Columns::add (const FileInfo *f) -> void
{
  let const width = (unicode::display_width (f->name)
                     + (Arguments::classify ? (file_indicator (*f) != 0) : 0)
                     + (Arguments::file_icons ? 2 : 0));
  let const is_quoted = ((Arguments::quoting == QuoteMode::default_)
                         && (f->name.front () == '\'' || f->name.front () == '"')
                         && (f->name.front () == f->name.back ()));

  let c = std::find_if (M_columns.begin (), M_columns.end (), [&](const Column &c){
    return c.elems.size () < M_rows;
  });

  if (c == M_columns.end ())
    {
      M_columns.emplace_back ().add (f, width, is_quoted);
    }
  else
    {
      c->add (f, width, is_quoted);
    }

  if (is_quoted)
    M_has_quoted = true;

  if (this->total_width () >= Arguments::width)
    {
      ++M_rows;
      this->reorder ();
      while (this->total_width () >= Arguments::width)
        {
          ++M_rows;
          this->reorder ();
        }
      M_columns.erase (
        std::remove_if (
          M_columns.begin (),
          M_columns.end (),
          [](const Column &col) {
            return col.elems.empty ();
          }
        ),
        M_columns.end ()
      );
    }
}

def Columns::print () -> void
{
  std::size_t r, c;
  for (r = 0; r < M_rows; ++r)
    {
      for (c = 0; c < M_columns.size (); ++c)
        {
          let const is_not_last = (c + 1) < M_columns.size ();
          let const &col = M_columns[c];
          let const &elem = col.elems[r];
          if (r >= col.elems.size ())
            break;
          if (elem.file->status_failed)
            std::fputs ("\x1b[2m", stdout);
          if (M_has_quoted)
            print_file_name (*elem.file, M_has_quoted, (col.width + elem.is_quoted) * is_not_last);
          else
            print_file_name (*elem.file, M_has_quoted, col.width * is_not_last);
          if (elem.file->status_failed)
            std::fputs ("\x1b[22m", stdout);
          if (is_not_last)
            {
              std::putchar (' ');
              std::putchar (' ');
            }
        }
      if (Arguments::file_icons && G_is_a_tty
          && M_rows >= G_term_height && r+1 == M_rows && c+1 == M_columns.size ())
        std::fputs ("\x1b[2m...\n\x1b[0m", stdout);
      else
        std::putchar ('\n');
    }
}

def Columns::total_width () const -> std::uint64_t
{
  let w = 0ull;
  for (let const &c : M_columns)
    w += c.width;
  w += (M_columns.size () - 1) * (2 + M_has_quoted);
  return w;
}

def Columns::new_column_count () const -> std::size_t
{
  let const o = M_columns.size ();
  let const old_cap = o * (M_rows - 1);
  let n = o / 2;
  let const new_cap = n * M_rows;
  n += (int)std::ceil ((float)(old_cap - new_cap) / M_rows);
  return n;
}

def Columns::reorder () -> void
{
  let cols = this->new_column_count ();

  let new_columns = arena::vector<Column> {};
  for (std::size_t i = 0; i < cols; ++i)
    new_columns.emplace_back ();

  let idx = std::make_pair (std::size_t {0}, std::size_t {0});
  for (let const &col : M_columns)
    {
      for (let const &elem : col.elems)
        {
          new_columns[idx.first].add (elem);
          this->next (idx);
        }
    }

  M_columns.swap (new_columns);
}

def Columns::next (std::pair<std::size_t, std::size_t> &idx) -> void
{
  ++idx.second;
  if (idx.second == M_rows)
    {
      ++idx.first;
      idx.second = 0;
    }
}
