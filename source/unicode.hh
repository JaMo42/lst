#pragma once
#include "stdafx.hh"

namespace unicode
{

class CodepointIterator
{
  CodepointIterator (const char8_t *b, const char8_t *e,
                     const char8_t *p, int s)
    : M_begin (b)
    , M_end (e)
    , M_pointer (p)
    , M_cp_size (s)
  {}

public:
  CodepointIterator (const std::string_view &str)
    : M_begin (reinterpret_cast<const char8_t *> (str.data ()))
    , M_end (M_begin + str.size ())
    , M_pointer (M_begin)
    , M_cp_size (0)
  {}
  CodepointIterator (const std::u8string_view &str)
    : M_begin (str.data ())
    , M_end (M_begin + str.size ())
    , M_pointer (M_begin)
    , M_cp_size (0)
  {}

  def begin () const -> CodepointIterator;
  def end () const -> CodepointIterator;

  def operator * () -> char32_t;

  def operator ++ () -> CodepointIterator &;
  def operator ++ (int) -> CodepointIterator;

  def operator != (const CodepointIterator &other) -> bool;

  def cp_size () const -> int { return M_cp_size; }

private:
  const char8_t *M_begin;
  const char8_t *M_end;
  const char8_t *M_pointer;
  // Size of the current codepoint (operator* must be called first)
  int M_cp_size;
};

def utf8_to_codepoint (const char8_t *bytes, int *cp_size = nullptr) -> char32_t;

static inline def utf8_to_codepoint (const char *bytes, int *cp_size = nullptr) -> char32_t
{ return utf8_to_codepoint (reinterpret_cast<const char8_t *> (bytes), cp_size); }

def display_width (char32_t ch) -> int;

def display_width (const arena::string &str) -> int;

// Used to adjust the padding amount in printf.
def padding_offset (const arena::string &str) -> int;

def path_to_str (const fs::path &p) -> arena::string;

}
