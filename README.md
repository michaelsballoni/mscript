# mscript
A simple, keyword-free scripting language for automating command line operations

It is useful for scripting that's too much for .bat files, and if Powershell or Python are unavailable or unnecessary

The thinking is, here's a simple scripting language, if it can solve your problem, then there's no need to get a bigger gun.

## a little taste
```
! Caching Fibonacci sequence
~ fib(n)
	! Check the cache
	? fib_cache.has(n)
		<- fib_cache.get(n)
	}
	
	! Compute the result
	$ fib_result
	? n <= 0
		& fib_result = 0
	? n = 1 || n = 2
		& fib_result = 1
	<>
		& fib_result = fib(n - 1) + fib(n - 2)
	}
	
	! Stash the result in the cache
	* fib_cache.add(n, fib_result)
	
	! All done
	<- fib_result
}
! Our cache is an index, a hash table, any-to-any
$ fib_cache = index()

! Print the first 10 values of the Fibonacci series
! Look, ma!  No keywords!
# n : 1 -> 10
	> fib(n)
}
```
It is a line-based, pseudo-object-oriented scripting language that uses symbols instead of keywords

## project layout
The mscript project is a Visual Studio solution.  mscript has no external dependencies; it statically links the runtime.

Here are the projects that make up the solution:
### mscript-lib
The mscript-lib project is where expressions and statements are implemented. All the working code in the solution is in this project.

- expressions
- object
- script_processor
- symbols
- utils
- vectormap

### mscript-tests

Unit tests

### mscript-test-runner

You can only do so much with unit tests without the test code getting large and unwieldy. Instead of making bad unit tests, I made a set of files with script to execute and results to expect. So in mscript-tests, you'll find test files, with statements up top and expected results below, separated by ===.

mscript-test-runner runs all scripts in the directory and validates that it gets the expected results.

### mscript

This is the script interpreter. All the code is in mscript-lib, so this is just a shell around that project.

The tricky bit in the mscript program is loading secondary scripts as requested by + statements. The path to the secondary script is relative to the script doing the importing. So if you tell the intepreter to run c:\my_scripts\mine.ms, and mine.ms imports yours.ms, yours.ms is looked for in c:\my_scripts, not in the current directory or some such thing.

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

## statements
```
/* a block
comment
*/

! a single-line comment, on its own line, can't be at the end of a line

> "print the value of an expression, like this string, including pi: " + round(pi, 4)

>> print exaclty what is on this line, allowing for any "! '= " 0!')* nonsense you'd like

{>>
every line
in "here"
is printed "as-is"
>>}

! Declare a variable with an optional initial value
! With no initial value, the variable has the null value
$ new_variable = "initial value"

! A variable assignment
! Once a variable has a non-null value, the variable cannot be assigned
! to a value of another type
! So mscript is somewhat dynamic typed
& new_variable = "some other value"

! The O signifies an unbounded loop, a while(true) type of thing
! All loops end in a closing curly brace, but do not start with an opening one
O
	...
	! the V statement is break
	> "gotta get out!"
	V
}

! If, else if, else
! No curly braces at ends of each if or else if clause, 
! just at the end of the overall statement
? some_number = 12
	& some_number = 13
? some_number = 15
	& some_number = 16
<>
	& some_number = -1
}

! A foreach loop
! list(1, 2, 3) creates a new list with the given items
! This statements processes each list item, printing them out
! Note the string promotion in the print line
@ item : list(1, 2, 3)
	> "Item: " + item
}

! An indexing loop
! Notice the pseudo-OOP of the my_list.length() and my_list.get() calls
! This is syntactic sugar for calls to global functions,
! length(my_list) and get(my_list, idx)
$ my_list = list(1, 2, 3)
# idx : 0 -> my_list.length() - 1
	> "Item: " + my_list.get(idx)
}

{
	! Just a little block statement for keeping variable scopes separate
	! Variables declared in here...
}
! ...are not visible out here

! Functions are declared like other statements
~ my_function (param1, param2)
	! do something with param1 and param2

	! Function return values...
	! ...with a value
	<- 15
	! ...without a value
	<-
}

! A little loop example
~ counter(low_value, high_value
	$ cur_value = low_value
	$ counted = list()
	O
		! Use the * statement to evaluate an expression and discard its return value
		! Useful for requiring deliberate ignoring of return values
		* counted.add(cur_value)
		& cur_value = cur_value + 1
		? cur_value > high_value
			! Use the V statement to leave the loop
			V
		<>
			! Use the ^ statement to go back up to the start of the loop, a continue statement
			^
		}
	}
	<- counted
}

! Load and run another script here, an import statement
! The script path is an expression, so you can dynamically load different things
! Scripts are loaded relative to the script they are imported from
! Scripts loaded in this way are processed just like top-level scripts,
! so they can declare global variables, define functions, and...execute script statements
! Plenty of rope...
+ "some_other_script.ms"
```
