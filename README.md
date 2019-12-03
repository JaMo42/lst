# lst

A directory listing tool for Windows similiar to Plan 9 ls.

## Usage

```
Usage:  lst [-adlmnqrstFLQT] [file...]
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
x  1 Administ Administ 1234MB Jan 13 07:11 afile.exe -> C:/path/to/target/target.exe
```

The columns represent:

- The file type letter
  - `-` - Normal
  - `?` - Unknown
  - `d` - Directory
  - `x` - Executable
  - `l` - Link or shortcut
  - `s` - System
  - `D` - Device
  - `v` - Virtual
- The number of links to the file
- Owner name (limited to 8 characters)
- Group name (limited to 8 characters)
- Size
- Time of last modification
- Name
- Link target (If the file is a link and the target exists)
