#include "natural_sort.hh"

static def compare_non_digit (char a, char b) -> int
{
  int c;
  if ((c = (a == '~') - (b == '~')) != 0)
    return c;
  if ((c = std::ispunct (a) - std::ispunct (b)) != 0)
    return c;
  return a - b;
}

static def compare_non_digit (std::string_view a, std::string_view b) -> int
{
  let const l = std::min (a.size (), b.size ());
  let c = 0;
  for (std::size_t i = 0; i < l; ++i)
    {
      if ((c = compare_non_digit (a[i], b[i])) != 0)
        return c;
    }

  return a.size () - b.size ();
}

def natural_compare (std::string_view a, std::string_view b) -> int
{
  let constexpr digits = "0123456789"sv;
  intmax_t c;
  std::size_t a_pos, b_pos;
  char *a_end, *b_end;
  // Special cases
  // Prioritize empty strings
  if ((c = b.empty () - a.empty ()) != 0)
    return c;
  // Prioritize strings starting with '.' to group dotfiles first
  if ((c = (b.front () == '.') - (a.front () == '.')) != 0)
    return c;
  // No need to care about the '.' and '..' files since we do not capture them
  // Compare first non-digit part
  a_pos = a.find_first_of (digits);
  b_pos = b.find_first_of (digits);
  if ((c = compare_non_digit (a.substr (0, a_pos), b.substr (0, b_pos))) != 0)
    return c;
  // Compare first digit part
  a.remove_prefix (a_pos);
  b.remove_prefix (b_pos);
  if ((c = (std::strtoull (a.data (), &a_end, 10)
            - std::strtoull (b.data (), &b_end, 10)))
      != 0)
    return c;
  // Recurse if either string has characters left
  a.remove_prefix (a_end - a.data ());
  b.remove_prefix (b_end - b.data ());
  if (a.size () + b.size ())
    return natural_compare (a, b);
  return 0;
}
