#pragma once

#include "object.h"
#include "vectormap.h"

namespace mscript
{
	/*
	Command-line argument processor function, parseArgs

	This function takes the list of command-line arguments (arguments),
	and specifications about how to process the arguments (argumentSpecs)

	The schema for argumentSpecs is a list of indexes
	Each index has pairs for "flag", "long-flag", "description", and "take"
	The first three are strings, like "-c", "--command", "command is a...command!"
	The last one, "takes", is a pair with a bool value specifying whether to get the next argument
	and have that be the value in the return value

	This function returns an index mapping the flag name to the value for that flag

	For flags with no value, the value is null
	The special non-flag "" gives a list of all un-tagged arguments

	Special flag -? / --help outputs the contents of argumentSpecs
	If you pass your own -? / --help function, this suppresses the special behavior
	*/
	object
		parseArgs
		(
			const object::list& arguments,
			const object::list& argumentSpecs
		);
}
