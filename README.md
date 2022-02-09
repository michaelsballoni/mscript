# mscript
A simple, keyword-free scripting language for automating command line operations

It is useful for scripting that's too much for .bat files, and if Powershell or Python are unavailable or not necessary.

The thinking is, here's a simple scripting language, if it can solve your problem, then there's no need to get a bigger gun.

## objects

In mscript, every variable contains an object (think .NET's Object)

An object can be one of six types of things:
1. null
2. number - double
3. string - wstring
4. bool
5. list - vector\<object\>
6. index - vectormap\<object, object\>

list and index are copied by reference, the rest by value

## expressions

```
Binary operators, from least to highest precedence:
or || and && != <= >= < > = % - + / * ^

Unary operators: - ! not

An expression can be:
null
true
false
number
string
dquote
squote
tab
lf
cr
crlf
pi
e
variable as defined by a $ statement

Strings can be double- or single-quoted, 'foo ("bar")' and "foo ('bar')" are valid; 
this is handy for building command lines that involve lots of double-quotes; 
just use single quotes around them.

String promotion:
	If either side of binary expression evaluates to a string, 
	the expression promotes both sides to string

Bool short-circuiting:
	The left expression is evaluated first
		If && and left is false, expression is false
		If || and left is true, expression is true

Standard math functions, for your math homework:
abs asin acos atan ceil cos cosh exp floor 
log log2 log10 round sin sinh sqrt tan tanh

getType(obj) - the type of an object obj as a string
			 - you can also say obj.getType()
			 - see the shorthand?
number(val)	 - convert a string or bool into a number
string(val)  - convert anything into a string
list(item1, item2, etc.) - create a new list with the elements passed in
index(key1, value1, key2, value2) - create a new index with the pairs of keys 
and values passed in

obj.clone() - deeply clone an object, including indexes containing list values, etc.
obj.length() - C++ .size(), string or list length, or index pair count

obj.add(to_add1, to_add2...) - append to a string, add to a list, or add pairs to an index
obj.set(key, value) - set a character in a string, change the value at a key in a list or index
obj.get(key) - return the character of a string, the element in a list, 
or the value for the key in an index
obj.has(value) - returns if string has substring, list has item, or index has key

obj.keys(), obj.values() - index collection access

obj.reversed() - returns copy of obj with elements reversed, including keys of an index
obj.sorted() - returns a copy of obj with elements sorted, including index keys

join(list_obj, separator) - join list items together into a string
split(str, separator) - split a string into a list of items

trim(str) - return a copy of a string with any leading or trailing whitespace removed
toUpper(str), toLower(string) - return a copy of a string in upper or lower case
str.replaced(from, to) - return a copy of a string with characters replaced

random(min, max) - return a random value in the range min -> max

obj.firstLocation(toFind), obj.lastLocation(toFind) - find the first or last location 
of an a substring in a string or item in a list
obj.subset(startIndex[, length]) - return a substring of a string or a slice of a list, 
with an optional length

obj.isMatch(regex) - see if a string is a match for a regular expression
obj.getMatches(regex) - return a list of matches from a regular expression applied to a string

exec(cmd_line) - execute a command line, return an index with keys 
("success", "exit_code", "output")
This is the main function gives mscript meaning in life.  
You build your command line, you call exec, and it returns an index with all you need to know. 
Write all the script you want around calls to exec, and get a lot done.

exit(exit_code) - exit the script with an exit code

error(error_msg) - raise an error, reported by the script interpreter

readFile(file_path, encoding) - read a text file into a string, using the specified encoding, 
either "ascii", "utf8", or "utf16"

writeFile(file_path, file_contents, encoding) - write a string to a text file with an encoding
```
