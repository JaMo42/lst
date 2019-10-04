# lst

A directory listing tool for Windows that is closer to (plan9)`ls` than Windows' default `dir` program.

# Usage

```
Usage:  lst [admnqrstFLQT] [file ...]
Arguments:
a       Show hidden files and files starting with `.`.
d       If argument is directory, list it, not its contents.
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
