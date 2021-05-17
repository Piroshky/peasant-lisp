# Peasant Lisp

An experimental lisp interpreter.



## departures from Common Lisp

### list splicing
`,@` can be used inside of backticked lists and regular lists.
This allows you to do something like this:

> (+ ,@'(1 2 3))
6

It also allows you to do silly things like this:

> (defsym myvar '(+ 1 2))
myvar

> (,@myvar 3)
6

## Available Functions and Macros

### Basic
defun
defmacro
expand
eval
defsym
set
let
progn
if
when
unless
cond
for-each
load
print

### List Manipulation
list
first
last
nth
pop
push
append
length
~

### Math
+
*

### Logic
=
<
<=
>
>=
and
or
not

### Bitwise
&
|
^
~
<<
>>