/ This sample computes file count for each extension for files in the current directory
 
/ Collect our command line parameter
$ dir_path = ""
? arguments.length() = 0
	& dir_path = "."
? arguments.length() = 1
	& dir_path = arguments.get(0)
<>
	* error("Either specify no parameters, or pass one directory path parameter")
}

/ Run the dir command to enumerate all paths
/ The /B gives us a bare list of full paths
/ The /S gives us all paths in the current directory and all subdirectories
>> Getting file paths...
$ exec_result = exec('dir /B /S "' + dir_path + '"')

/ Extract a list of paths from the raw dir output
>> Processing file paths...
$ paths = split(replaced(exec_result.get("output"), crlf, lf), lf)

/ Loop over the paths collecting extension stats
/ And index is like a Scripting.Dictionary
$ ext_stats = index()
@ path : paths
	/ Find the last dot
	/ Use the ^ command to skip to the next path in the list
	$ dot_index = path.lastLocation('.')
	? dot_index < 0
		^
	}
	
	/ Extract the file extension
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

/ Find which extensions have which counts
$ sorted_counts = reversed(sorted(ext_stats.values()))
$ unique_counts = index()
@ count : sorted_counts
	* unique_counts.set(count, 0)
}

/ Output what we've found
>>
> "File extensions: " + ext_stats.length()
@ count : unique_counts
	@ ext : ext_stats
		? ext_stats.get(ext) = count
			> ext + ": " +  count
		}
	}
}
