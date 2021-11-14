#include "unicode.hh"

namespace unicode
{

def CodepointIterator::begin () const -> CodepointIterator
{
  return CodepointIterator (M_begin, M_end, M_begin, M_cp_size);
}

def CodepointIterator::end () const -> CodepointIterator
{
  return CodepointIterator (M_begin, M_end, M_end, M_cp_size);
}

def CodepointIterator::operator * () -> char32_t
{
  return utf8_to_codepoint (M_pointer, &M_cp_size);
}

def CodepointIterator::operator ++ () -> CodepointIterator &
{
  M_pointer += (1 + (M_pointer[0] >= 0x80) + (M_pointer[0] >= 0xe0)
                + (M_pointer[0] >= 0xf0));
  return *this;
}

def CodepointIterator::operator ++ (int) -> CodepointIterator
{
  let const copy = *this;
  this->operator++ ();
  return copy;
}

def CodepointIterator::operator != (const CodepointIterator &other) -> bool
{
  return M_pointer != other.M_pointer;
}

def utf8_to_codepoint (const char8_t *bytes, int *cp_size) -> char32_t
{
  if (bytes[0] < 0x80)
    {
      if (cp_size) *cp_size = 1;
      return bytes[0];
    }
  else if (bytes[0] < 0xe0)
    {
      if (cp_size) *cp_size = 2;
      return (char32_t (bytes[0] & 0x1f) << 6) | (bytes[1] & 0x3f);
    }
  else if (bytes[0] < 0xf0)
    {
      if (cp_size) *cp_size = 3;
      return ((char32_t (bytes[0] & 0xf) << 12)
              | (char32_t (bytes[1] & 0x3f) << 6)
              | (bytes[2] & 0x3f));
    }
  else
    {
      if (cp_size) *cp_size = 4;
      return ((char32_t (bytes[0] & 0x7) << 18)
              | (char32_t (bytes[1] & 0x3f) << 12)
              | (char32_t (bytes[2] & 0x3f) << 6)
              | (bytes[3] & 0x3f));
    }
}

// TODO: display_width, padding_offset, CodepointIterator
// Find out why the iterator includes the terminating null byte and remove
// special handling from display_width and padding_offset.
// Note: display_width (char32_t) needs this now (for file_name_width)

def display_width (char32_t ch) -> int
{
  // Note: For performance this does not check for zero-width characters.
  if (ch == 0)
    return 0;
  return (1
          + (ch >= 0x1100
              && (ch <= 0x115f || ch == 0x2329 || ch == 0x232a
                  || (ch >= 0x2e80 && ch <= 0xa4cf && ch != 0x303f)
                  || (ch >= 0xac00 && ch <= 0xd7a3)
                  || (ch >= 0xf900 && ch <= 0xfaff)
                  || (ch >= 0xfe10 && ch <= 0xfe19)
                  || (ch >= 0xfe30 && ch <= 0xfe6f)
                  || (ch >= 0xff00 && ch <= 0xff60)
                  || (ch >= 0xffe0 && ch <= 0xffe6)
                  || (ch >= 0x20000 && ch <= 0x2fffd)
                  || (ch >= 0x30000 && ch <= 0x3fffd))));
}

def display_width (const arena::string &str) -> int
{
  let w = 0;
  let const iter = CodepointIterator (str);
  for (let c : iter)
    w += display_width (c);
  return w;
}

def padding_offset (const arena::string &str) -> int
{
  let iter = CodepointIterator (str);
  let o = 0;
  // For some reason CodepointIterator::M_cp_size does not get set when using
  // the range based for loop
  for (; iter != iter.end (); ++iter)
    {
      let const c = *iter;
      if (c)
        o += iter.cp_size () - display_width (c);
    }
  return o;
}

// TODO: consider adding 'path_to_str_temp' converting the path into a static
// string in the function adn returning a reference to it, since when creating
// a FileInfo the returned string gets immediately processed into another string
// anyways.
def path_to_str (const fs::path &p) -> arena::string
{
  // On Windows, std::filesystem::path::string raises an encoding error if the
  // path contains non-ascii characters.
#ifdef _WIN32
  return arena::string (reinterpret_cast<const char *> (p.u8string ().c_str ()));
#else
  return p.string<char> (arena::allocator<char> {});
#endif
}

}
