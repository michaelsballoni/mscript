/*
This sample program reads lines from an input file and writes them to a 4db NoSQL database
Once the database is loaded, the user enters search terms and gets matching lines
This sample demonstrates command line processing, file I/O, and 4db database programming
Note the extensive use of \ line continuations: mscripts may be line-based, 
but they doesn't have to be ugly!
*/

+ "mscript-db.dll"

/ Define the command line flags we expect as arguments specifications
/ The arguments specifications are represented as a list of indexes
$ arg_specs = \
list \
( \
	index \
	( \
		"flag", "-in", \
		"long-flag", "--input_file_path", \
		"description", "Path of the input file to process", \
		"takes", true, \
		"required", true \
	), \	
	index \
	( \
		"flag", "-db", \
		"long-flag", "--database_file_path", \
		"description", "Path for the database file", \
		"takes", true, \
		"required", true \
	), \
	index \
	( \
		"flag", "-enc", \
		"long-flag", "--text_encoding", \
		"description", "The encoding of the input text file", \
		"takes", true, \
		"required", true, \
		"default", "utf-8" \
	) \
)
$ args = parseArgs(arguments, arg_specs)
$ input_file_path = args.get("-in")
$ database_file_path = args.get("-db")
$ text_encoding = args.get("-enc")

>>

>> Command line arguments:
> "Input file:     " + input_file_path
> "Database file:  " + database_file_path
> "Text encoding:  " + text_encoding
! err
	> "Error processing command line parameters: " + err
	exit(1)
}

>>

>> Reading lines from input file...
$ file_lines = readFileLines(input_file_path, text_encoding)
! err
	> "Error reading input file: " + err
	exit(1)
}
> "File Line Count: " + file_lines.length()

>> Getting 4db database up and running...
exec('del "' + database_file_path + '"') // fresh DB file every time
msdb_4db_init("db", database_file_path)
! err
	> "Error initializing database: " + err
	exit(1)
}

>> Importing lines into database...
++ f : 0 -> file_lines.length() -1
	/ NOTE: Using line as the primary key did not handle identical lines,
	/ 		hence the line number
	$ line = file_lines.get(f)
	$ line_number = f + 1
	msdb_4db_define("db", "lines", line_number + ": " + line, index("len", length(line)))
}
! err
	> "Error importing lines into database: " + err
	exit(1)
}


>> Database loaded, ready to query!
O // loop forever...until Ctrl-C at least
	>>
	>> Enter your search criteria:
	$ criteria = trimmed(input())
	? criteria.length() == 0
		^
	}
	>>
	> "Criteria: " + criteria
	>>
	
	/ Get the results, using the "value" column, which is the primary key
	/ which is the text of the line, to SELECT out, and to MATCH the criteria
	/ We sort by id for top-to-bottom file output
	/ The criteria is passed into msdb_4db_query as a param->value index
	$ results = \
	msdb_4db_query \
	( \
		"db", \
		"SELECT value, id FROM lines WHERE value MATCHES @criteria ORDER BY id", \
		index("@criteria", criteria) \
	)
	
	/ Process the results
	$ result_count = results.length()
	> "Results: " + (result_count - 1) // ignore the column headers "result"
	++ idx : 1 -> result_count - 1
		$ line = results.get(idx)
		> line.get(0) // Ignore the id column which was just needed for sorting
	}
	! err
		> "Error getting search results: " + err
	}
}
