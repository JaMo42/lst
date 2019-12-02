# lst

[![CodeFactor](https://www.codefactor.io/repository/github/jamo42/lst/badge)](https://www.codefactor.io/repository/github/jamo42/lst)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/78dbf191dfa54757a887badcc010cc01)](https://www.codacy.com/manual/JaMo42/lst?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=JaMo42/lst&amp;utm_campaign=Badge_Grade)

A directory listing tool similiar to Plan 9 ls.

## Usage

```
Usage:  lst [-admnqrstFLQT] [file...]
Arguments:
a       Show hidden files and files starting with `.`.
d       If argument is directory, list it, not its contents.
l       Use a long listing format.
m       Do not colorize the output.
n       Do not sort the listing.
q       Always quote file names.
r       Reverse sorting order.
s       Sort by file size.
t       Sort by time modified.
F       Add a `\` after all directory names.
L       Seperate files using spaces instead of newlines.
Q       Never quote file names.
T       Sort by time created.
```

## Long listing format

```
  1 Administrator Administrator 1234MB Jan 13 07:11 afile.exe -> C:/path/to/target/target.exe
```

The columns represent:

- The number of links to the file
- Owner name
- Group name
- Size
- Time of last modification
- Name
- Link target (If the file is a link and the target exists)
