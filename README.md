# mscript
A simple, symbol-driven scripting language for automating command line operations

It is useful for scripting that's too much for .bat files, when Powershell or Python are unavailable or unnecessary

The thinking is, here's a simple scripting language, if it can solve your problem, then there's no need to get a bigger gun

To see the language reference, visit [mscript.io](https://mscript.io)

## a little taste
```
/ Import the timestamp DLL for working with file timestamps
+ "mscript-timestamp.dll"

/ Make sure we got a file path argument
? arguments.length() != 1
	>> Provide the path to the file to touch
	exit(0)
}

/ Report on the file path and its timestamp
$ file_path = arguments.get(0)
> "File: " + file_path
> "Last Modified Be4: " + msts_last_modified(file_path)

/ Do the deed
msts_touch(file_path)

/ Report on the new timestamp
> "Last Modified Now: " + msts_last_modified(file_path)

```
It is a line-based, pseudo-object-oriented scripting language that uses symbols instead of keywords

To jump in and see more mscript, check out some [sample scripts](mscript-examples)

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

In mscript-test-scripts you'll find test files, with statements up top and expected results below, separated by a === line

mscript-test-runner runs all scripts in the directory and validates that it gets the expected results

### mscript

This is the script interpreter; all the code is in mscript-core and mscript-lib, so this is just a shell around that project
