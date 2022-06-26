#include "columns.hh"

Columns::Columns ()
  : M_columns {}
  , M_rows (1)
  , M_has_quoted (false)
{}

def Columns::add (const FileInfo *f) -> void
{
  let const width = (unicode::display_width (f->name)
                     + (Arguments::classify ? (file_indicator (*f) != 0) : 0));
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
    }
}

def Columns::print () -> void
{
  static arena::string sep;

  for (std::size_t r = 0; r < M_rows; ++r)
    {
      for (std::size_t c = 0; c < M_columns.size (); ++c)
        {
          let const is_not_last = (c + 1) < M_columns.size ();
          let const &col = M_columns[c];
          let const &elem = col.elems[r];
          if (r >= col.elems.size ())
            {
              if (r+1 == M_rows && c+1 == M_columns.size ())
                std::fputs ("\x1b[2m...\x1b[0m", stdout);
              continue;
            }
          if (M_has_quoted)
            {
              if (!elem.is_quoted)
                std::putchar (' ');
              print_file_name (*elem.file, (col.width + elem.is_quoted) * is_not_last);
            }
          else
            print_file_name (*elem.file, col.width * is_not_last);
          if (is_not_last)
            std::fputs (sep.assign (S_separator, ' ').c_str (), stdout);
        }
      std::putchar ('\n');
    }
}

def Columns::total_width () const -> std::uint64_t
{
  let w = 0ull;
  for (let const &c : M_columns)
    w += c.width;
  w += (M_columns.size () - 1) * (S_separator + M_has_quoted);
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
