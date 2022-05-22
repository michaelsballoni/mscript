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
		"flag", "-rc", \
		"long-flag", "--recompute", \
		"description", "Should the file size data be recomputed?", \
		"default", false \
	) \
)
$ args = parseArgs(arguments, arg_specs)
$ starting_dir_path = args.get("-dir")
$ db_file_path = args.get("-db")
$ log_file_path = args.get("-log")
$ log_level = args.get("-ll")
$ recompute = string(args.get("-rc")) == "true"
>>
>> Configuration:
> "Dir Path:      " + starting_dir_path
> "DB File Path:  " + db_file_path
> "Log File Path: " + log_file_path
> "Log Level:     " + log_level
> "Recompute:     " + recompute
>>
! err
	> "Error processing command line arguments: " + err
	exit(1)
}

/ Start logging to a fresh file
exec('del "' + log_file_path + '"', index("method", "system")) // system doesn't print
mslog_start(index("filename", log_file_path))
mslog_setlevel(log_level)

$ did_db_file_exist = false
{
	$ exec_result = exec('dir "' + db_file_path + '"')
	did_db_file_exist = firstLocation(trimmed(exec_result.get("output")), "File Not Found") = -1
}

? recompute
	exec('del "' + db_file_path + '"', index("method", "system")) // system doesn't print anything
}

$ db = msdb_sql_init("db", db_file_path)

? recompute
	msdb_sql_exec("db", "CREATE TABLE file_sizes (FilePath STRING NOT NULL, SizeBytes NUMBER NOT NULL)")
	processDir(starting_dir_path)
}

/ Do the deed

/ Close up shop
msdb_sql_close("db")

>> All done.

~ processDir(dirPath)
	mslog_debug("DIR Path: " + dirPath)
	> dirPath

	$ dir_result = exec('dir "' + dirPath + '"')
	$ dir_output = dir_result.get("output")
	$ dir_output_lines = splitLines(dir_output)
	? log_level = "DEBUG"
		mslog_debug("DIR Output:" + crlf + dir_output)
	}
	
	$ found_dirs = list()
	$ found_file_sizes = index()
	@ line : dir_output_lines
		/ Skip header lines
		? line.firstLocation(" ") == 0
			^
		}
		
		/ Match up the DIR parts
		$ pattern = \
			"[0-9\/]+" + \
			"\s*" + \
			"[0-9\:]+\s*(AM|PM)" + \
			"\s*" + \
			"((\<DIR\>\s*)|([0-9\,]+))" + \
			"\s*" + \
			"([\S]*)"
		$ matches = line.getMatches(pattern, true)
		? matches.length() < 2 // file size and path stem
			^
		}

		$ path_stem = trimmed(matches.get(matches.length() - 1))
		? path_stem = "." OR path_stem = ".."
			^
		}
		
		$ full_path = dirPath + "\" + path_stem
		
		$ len_str = replaced(trimmed(matches.get(matches.length() - 2)), ",", "")
		? len_str.length() == 0
			found_dirs.add(full_path)
		}
		<>
			found_file_sizes.add(full_path, number(len_str))
		}
	}
	/ FORNOW DEBUG
	> "Dirs:  " + found_dirs
	> "Files: " + found_file_sizes
	! err
		> "Error processing directory: " + err
		exit(1)
	}
	
	/ load file size data into the DB

	/ recurse on the dirs
}
