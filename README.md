# ezed

### Commands
```text
Commands:
q                      quit
h                      display this
s                      save to the file
v                      display infos about the file and others
e [l] [txt]            changes the text of the line [l] to [txt]
w [l] [txt]            changes the text of the line [l] to original indent + [txt]
i [l]                  insert empty line after line [l]
i [l] [txt]            insert line with text [t] after line [l]
d [l: range]           delete all line(s) in the range [l]
a                      append empty line
a [txt]                append line with text [txt]
l                      list all lines
l [l: range]           lists all lines in the range [l]
m [l]                  get indent (spaces) of line [l]
c [l] [a]              copy line [l] and insert it after line [a]
p [l] [a]              move line [l] after line [a]
f [txt]                finds occurrences of [txt]
x                      list all contents in the find buffer
y                      clear the find buffer
* [l: range]           removes every element in the find buffer where the line IS NOT in the range [l]
& [l: range]           removes every element in the find buffer where the line IS in the range [l]
$ [txt]                finds occurrences of [txt] in the find buffer and overwrites the find buffer with it
% [txt]                removes every element in the find buffer which contains [txt]
r [txt]                replaces everything in the find buffer with [txt]
o [name][args][body..] defines a macro
t [macro] [args..]     executes a macro

Ranges:
a-b                    range from a to b
a-                     range from a to end
-b                     range from 0 to b
a                      range of a

```
