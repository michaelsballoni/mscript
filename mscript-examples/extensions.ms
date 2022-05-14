/ This sample computes the file count for each extension of the files in a directory
 
/ Collect our command line parameter for the directory path
/ No parameters -> use current directory
/ One parameter -> that's the directory to process
/ Otherwise, error out
$ dir_path = ""
? arguments.length() = 0
	& dir_path = "."
? arguments.length() = 1
	& dir_path = arguments.get(0)
<>
	>> Either specify no parameters, or pass one directory path parameter
	* exit(0)
}

/ Run the dir command to enumerate all paths
/ The /B gives us a bare list of full paths
/ The /S gives us all paths in the directory and all of its subdirectories
>> Getting file paths...
$ exec_result = exec('dir /B /S "' + dir_path + '"')

/ Extract a list of paths from the raw dir output
>> Processing file paths...
$ paths = splitLines(exec_result.get("output"))

/ Loop over the paths collecting extension stats
/ An index is like a Scripting.Dictionary
$ ext_stats = index()
@ path : paths
	/ Find the last dot
	/ Failing that use the ^ command to skip to the next path in the list
	$ dot_index = path.lastLocation('.')
	? dot_index <= 0
		^
	}
	
	/ Extract the file extension, skip ahead if not as expected
	$ ext = toLower(path.subset(dot_index + 1))
	? ext.length() < 2 || ext.get(1) = '.' || ext.has('\')
		^ 
	}
	
	/ Add to our stats
	? ext_stats.has(ext)
		* ext_stats.set(ext, ext_stats.get(ext) + 1)
	<>
		* ext_stats.set(ext, 1)
	}
}

/ Get the unique extension counts so we can present a list, 
/ highest count to lowest
/ Take the values, the counts of each extension,
/ sort this lowest to highest,
/ reverse that to go highest to lowest.
/ Loop over that adding to an index
/ making for a unique set of count values with the counts as keys
$ unique_counts = index()
@ count : reversed(sorted(ext_stats.values()))
	* unique_counts.set(count, 0)
}

/ Output what we've found
/ Loop over the counts output extensions that have that count
>>
> "File extensions: " + ext_stats.length()
@ count : unique_counts
	@ ext : ext_stats
		? ext_stats.get(ext) = count
			> ext + ": " +  count
		}
	}
}
