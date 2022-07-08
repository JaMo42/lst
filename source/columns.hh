#pragma once
#include "args.hh"
#include "lst.hh"

extern unsigned G_term_height;

class Columns
{
  struct Element
  {
    const FileInfo *file;
    unsigned width;
    bool is_quoted;

    Element (const FileInfo *f, unsigned w, bool q)
      : file (f), width (w), is_quoted (q)
    {}
  };

  struct Column
  {
    arena::vector<Element> elems;
    unsigned width;

    Column ()
      : elems {}, width (0)
    {}

    def add (const Element &elem) -> void
    {
      elems.push_back (elem);
      if ((elem.width - elem.is_quoted) > width)
        width = elem.width - elem.is_quoted;
    }

    def add (const FileInfo *f, unsigned w, bool q)
    {
      elems.emplace_back (f, w, q);
      if ((w - q) > width)
        width = w - q;
    }
  };

public:
  Columns ();

  def add (const FileInfo *file) -> void;

  def print () -> void;

private:
  def total_width () const -> std::uint64_t;

  def new_column_count () const -> std::size_t;

  def reorder () -> void;

  // Set `idx` to the column/row index of the next cell
  // idx.first -> col, idx.second -> row
  def next (std::pair<std::size_t, std::size_t> &idx) -> void;

private:
  // Use `separator` spaces as separator
  static constexpr int S_separator = 2;

  arena::vector<Column> M_columns;
  std::size_t M_rows;
  bool M_has_quoted;
};
