/*
This sample determines the size of files in a given directory
and then the user can get file size stats
The progress in generating this data is written to a log file
*/
+ "mscript-log.dll"
+ "mscript-db.dll"

$ arg_specs = \
list \
( \
	index \
	( \
		"flag", "-dir", \
		"long-flag", "--directory", \
		"description", "What directory should be processed to get file sizes?", \
		"takes", true, \
		"default", "." \
	), \
	index \
	( \
		"flag", "-db", \
		"long-flag", "--database-file-path", \
		"description", "Where should the file sizes database be put?", \
		"takes", true, \
		"default", "file-sizes.db" \
	), \
	index \
	( \
		"flag", "-log", \
		"long-flag", "--log-file-path", \
		"description", "Where should log output be written?", \
		"takes", true, \
		"default", "file-sizes.log" \
	), \
	index \
	( \
		"flag", "-ll", \
		"long-flag", "--log-level", \
		"description", "What level to log at? (DEBUG, INFO, ERROR, or NONE)", \
		"takes", true, \
		"default", "INFO" \
	), \
	index \
	( \
		"flag", "-rebuild", \
		"long-flag", "--rebuild-index", \
		"description", "Should the program start over and rebuild the index?", \
		"default", false \
	) \
)
$ args = parseArgs(arguments, arg_specs)

$ starting_dir_path = args.get("-dir")
$ db_file_path = args.get("-db")
$ log_file_path = args.get("-log")
$ log_level = args.get("-ll")
$ rebuild = args.get("-rebuild")

>
> "Configuration:"
> "Dir Path:      " + starting_dir_path
> "DB File Path:  " + db_file_path
> "Log File Path: " + log_file_path
> "Log Level:     " + log_level
> "Rebuild:       " + rebuild
>
! err
	> "Processing command line arguments failed: " + err
	* exit(1)
}

/ Start logging to a fresh file every time
>!
>> fmt('del "{0}"', log_file_path)
* mslog_start(log_file_path, log_level)
~ logError(msg)
	* mslog_error(msg)
	> "ERROR: " + msg
}
~ logMsg(msg)
	* mslog_info(msg)
	> msg
}
~ logDebug(msg)
	* mslog_debug(msg)
}

$ did_db_file_exist = false
{
	>> fmt('IF EXIST "{0}" ECHO exists', db_file_path)
	* did_db_file_exist = trimmed(ms_CommandOutput) == "exists"
}

? !did_db_file_exist
	* rebuild = true
}

? rebuild
	>!
	>> fmt('del "{0}"', db_file_path)
	
	* logMsg("Processing files and folders...")
	* msdb_sql_init("db", db_file_path)
	* msdb_sql_exec \
	( \
		"db", \
		"CREATE TABLE file_sizes (FilePath STRING NOT NULL, SizeBytes NUMBER NOT NULL)" \
	)
	* processDir(starting_dir_path)
	* msdb_sql_close("db")
}
* msdb_sql_init("db", db_file_path)
! err
	* logError("Setting up index failed: " + err)
	* exit(1)
}

O // UI loop
	>
	> "Enter the path pattern to compute stats.  Like *.mp3"
	>
	$ pattern = trimmed(input())
	$ sql_pattern = "%" + pattern.replaced("*", "%")
	$ sql_query = \
		"SELECT COUNT(*), SUM(SizeBytes) FROM file_sizes WHERE FilePath LIKE @like"
	$ results = msdb_sql_exec("db", sql_query, index("@like", sql_pattern))
	? results.length() <= 1
		> No results
		^
	}
	$ result = results.get(1)

	>
	$ file_count = result.get(0)
	? file_count = null
		* file_count = 0
	}
	* logMsg("Count:     " + file_count)
	/       Size (GB): 
	
	$ size_bytes = result.get(1)
	? size_bytes = null
		* size_bytes = 0
	}
	$ size_str = ""
	? size_bytes > 1024 * 1024 * 1024
		* size_str = "Size (GB): " + round(size_bytes / 1024 / 1024 / 1024, 2)
	}
	? size_bytes > 1024 * 1024
		* size_str = "Size (MB): " + round(size_bytes / 1024 / 1024, 2)
	}
	<>
		* size_str = "Size (KB): " + round(size_bytes / 1024, 2)
	}
	* logMsg(size_str)
	
	! err
		> "Querying interface failed: " + err
		^
	}
}

/ Unreachable...
* msdb_sql_close("db")
* mslog_stop()
> All done.

/*
Implementation of search index population
Recursive function that runs DIR at each level and parses out files to add to index
and directories to recurse on
*/
~ processDir(dirPath)
	* logDebug("DIR Path: " + dirPath)
	> dirPath
	
	$ dir_output_lines = null
	{
		$ dir_result = exec(fmt('dir "{0}"', dirPath))
		$ dir_output = dir_result.get("output")
		* dir_output_lines = splitLines(dir_output)
	}
	
	$ found_dirs = list()
	$ found_file_sizes = index()
	$ line_pattern = \
		"[0-9\/]+" + \
		"\s*" + \
		"[0-9\:]+\s*(AM|PM)" + \
		"\s*" + \
		"((\<DIR\>\s*)|([0-9\,]+))" + \
		"\s*" + \
		"(\S.*)"
	@ line : dir_output_lines
		/ Skip header lines
		? line.firstLocation(" ") == 0
			^
		}
		
		/ Match up the DIR parts
		$ matches = line.getMatches(line_pattern, true)
		// > "DEBUG: matches: " + matches
		? matches.length() < 2 // get at least the file size and path stem
			^
		}

		$ path_stem = trimmed(matches.get(matches.length() - 1))
		? path_stem = "." OR path_stem = ".."
			^
		}
		
		$ full_path = path_stem
		? dirPath <> "."
			* full_path = dirPath + "\" + full_path
		}
		
		$ len_str = replaced(trimmed(matches.get(matches.length() - 2)), ",", "")
		? len_str.length() == 0
			* found_dirs.add(full_path)
		}
		<>
			* found_file_sizes.add(full_path, number(len_str))
		}
	}
	! err
		> fmt('Error processing directory "{0}": {1}', dirPath, err
		* exit(1)
	}
	
	/ load the file size data we found into the DB
	$ insert_statement = \
		"INSERT INTO file_sizes (FilePath, SizeBytes) VALUES (@filePath, @sizeBytes)"
	$ insert_params = index()
	@ file_path : found_file_sizes
		* insert_params.set("@filePath", file_path)
		* insert_params.set("@sizeBytes", found_file_sizes.get(file_path))
		* msdb_sql_exec("db", insert_statement, insert_params)
	}

	/ recurse on the dirs we found
	@ dir_path : found_dirs
		* processDir(dir_path)
	}
}
