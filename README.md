# mscript
A simple, symbol-driven scripting language for automating command line operations

It is useful for scripting that's too much for .bat files, when Powershell or Python are unavailable or unnecessary

The thinking is, here's a simple scripting language, if it can solve your problem, then there's no need to get a bigger gun

## a little taste
```
// Caching Fibonacci sequence
~ fib(n)
	// Check the cache
	? fib_cache.has(n)
		<- fib_cache.get(n)
	}
	
	// Compute the result
	$ fib_result
	? n <= 0
		& fib_result = 0
	? n <= 2
		& fib_result = 1
	<>
		& fib_result = fib(n - 1) + fib(n - 2)
	}
	
	// Stash the result in the cache
	* fib_cache.add(n, fib_result)
	
	! All done
	<- fib_result
}
// Our cache is an index, a hash table, any-to-any
$ fib_cache = index()

// Print the first 10 values of the Fibonacci series
// Look, ma!  No keywords!
# n : 1 -> 10
	> fib(n)
}
```
It is a line-based, pseudo-object-oriented scripting language that uses symbols instead of keywords

To jump in and see more mscript, check out the [musicdb sample](mscript-examples/musicdb.ms)

## project layout
The mscript project is a Visual Studio solution.  mscript has no external dependencies; it statically links the runtime

Here are the projects that make up the solution:

### mscript-core
The mscript-core project contains the minimum necessary function for DLL integraion

- object
- object_json
- module_utils
- utils
- vectormap

### mscript-lib
The mscript-lib project is where expressions and statements are implemented

- expressions
- script_processor
- symbols

### mscript-tests

Unit tests

### mscript-test-runner

You can only do so much with unit tests without the test code getting large and unwieldy

Instead of making bad unit tests, I made a set of files with script to execute and results to expect

In mscript-test-scripts you'll find test files, with statements up top and expected results below, separated by ===

mscript-test-runner runs all scripts in the directory and validates that it gets the expected results

### mscript

This is the script interpreter; all the code is in mscript-core and mscript-lib, so this is just a shell around that project

The tricky bit in the mscript program is loading secondary scripts as requested by + statements

The path to the secondary script is relative to the script doing the importing

So if you tell the intepreter to run c:\my_scripts\mine.ms, and mine.ms imports yours.ms, yours.ms is looked for in c:\my_scripts, not in the current directory or some such thing

## next steps
To see the language reference, visit [mscript.io](https://mscript.io){:target="_blank"}

To read about the development of mscript, go to CodeProject for [version 1.0](https://www.codeproject.com/Articles/5324522/mscript-A-Programming-Language-for-Scripting-Comma){:target="_blank"}
