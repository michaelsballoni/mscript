+ "mscript-log.dll"

$ log_level = ""

& log_level = mslog_getlevel()
> "Initial log level: " + log_level

>> Starting logging...
* mslog_start(index("filename", "logger.log", "prefix", "%t - %l"))
& log_level = mslog_getlevel()
> "NONE log level: " + log_level

* mslog_setlevel("INFO")
& log_level = mslog_getlevel()
> "INFO log level: " + log_level

* mslog_setlevel("DEBUG")
& log_level = mslog_getlevel()
> "DEBUG log level: " + log_level

* mslog_error("Test error")
* mslog_info("Test info")
* mslog_debug("Test debug")

>> Stopping logging...
* mslog_stop()
& log_level = mslog_getlevel()
> "NONE log level: " + log_level

$ log_file_lines = split(replaced(readFile("logger.log", "utf-8"), crlf, lf), lf)

$ error_seen = false
$ info_seen = false
$ debug_seen = false

@ log_line : log_file_lines
	> log_line
	? log_line.has("ERROR")
		& error_seen = true
	}
	? log_line.has("INFO")
		& info_seen = true
	}
	? log_line.has("DEBUG")
		& debug_seen = true
	}
}

? !error_seen
	* error("ERROR not found")
}
? !info_seen
	* error("INFO not found")
}
? !debug_seen
	* error("DEBUG not found")
}

>> All done.