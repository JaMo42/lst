#include "match.hh"

static def range_match (std::string_view pattern, char test) -> std::size_t
{
  let ok = false;
  let negate = false;
  let i = std::size_t (0);

  if (pattern[i] == '^' || pattern[i] == '!')
    {
      negate = true;
      ++i;
    }

  while (pattern[i] != ']')
    {
      if (ok)
        ++i;
      else if (pattern[i] == '-')
        {
          if (test >= pattern[i-1] && test <= pattern[i+1])
            ok = true;
          i += 2;
        }
      else
        {
          if (pattern[i] == '\\')
            ++i;
          if (pattern[i] == test)
            ok = true;
          ++i;
        }
    }

  return ok == negate ? 0 : i + 1;
}

def match (std::string_view pattern, std::string_view string) -> bool
{
  std::size_t i = 0, m = 0;
  while (i < pattern.size ())
    {
      switch (pattern[i])
        {
        case '?':
          {
            ++i;
            if (m == string.size ())
              return false;
            ++m;
          }
          break;

        case '*':
          {
            while (i < pattern.size () && pattern[i] == '*')
              ++i;
            if (i == pattern.size ())
              return true;
            while (m < string.size ())
              {
                if (match (pattern.substr (i), string.substr (m)))
                  return true;
                ++m;
              }
            return false;
          }
          break;

        case '[':
          {
            let const ii = range_match (pattern.substr (i), string[m]);
            if (ii)
              {
                i += ii;
                ++m;
              }
            else
              return false;
          }
          break;

        default:
          {
            if (pattern[i] == '\\')
              ++i;
            if (string[m] != pattern[i])
              return false;
            ++i;
            ++m;
          }
          break;
        }
    }
  return m == string.size ();
}
