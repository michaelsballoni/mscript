/*
Command-line argument processor function, parseArgsScript
Pseudocode for the built-in function parseArgs,
and a good example of a non-trivial user-defined function

This function takes the list of command-line arguments (arguments),
and specifications about how to process the arguments (argumentSpecs)

The schema for argumentSpecs is a list of indexes
Each index has pairs for "flag", "long-flag", "description", and "take"
The first three are strings, like "-c", "--command", "command is a...command"
The last one, "takes", is a pair with a bool value specifying whether to get the next argument
and have that be the value in the return value

This function returns an index mapping the flag name to the value for that flag
For flags with no value, the value is null
The special non-flag "" gives a list of all un-tagged arguments

This function also takes the version of the mscript EXE that is processing the arguments

-v / --version outputs the mscript EXE version

-? / --help outputs the contents of argumentSpecs
*/
~ parseArgsScript(arguments, argumentSpecs, exeVersion)
	$ ret_val = index()
	
	/ Add the list for the raw, non-flag or flag-related arguments
	$ raw_arg_list = list()
	* ret_val.add("", raw_arg_list)
	
	/ Validate the arguments list
	? getType(arguments) != "list"
		* error("arguments must be a list")
	}
	@ cur_arg : arguments
		? getType(cur_arg) != "string"
			* error("All arguments must be strings")
		}
	}
	
	/ Put the -? flag into the mix, if not already there
	$ already_had_help = false
	@ cur_spec : argumentSpecs
		? cur_spec.get("flag") == "-?" || cur_spec.get("long-flag") == "--help"
			& already_had_help = true
			V
		}
	}
	? !already_had_help
		* argumentSpecs.add(index("flag", "-?", \
								  "long-flag", "--help", \
								  "description", "Get the usage info about this script"))
	}
			
	/ Put the -v flag into the mix, if not already there
	$ already_had_version = false
	@ cur_spec : argumentSpecs
		? cur_spec.get("flag") == "-v" || cur_spec.get("long-flag") == "--version"
			& already_had_version = true
			V
		}
	}
	? !already_had_version
		* argumentSpecs.add(index("flag", "-v", \
								  "long-flag", "--version", \
								  "description", "Get the version of mscript running this script"))
	}
	
	/ Validate the argument specifications
	@ cur_spec : argumentSpecs
		? getType(cur_spec) != "index"
			* error("All argumentSpecs must be indexes")
		}
		? !cur_spec.has("flag") \
		  || getType(cur_spec.get("flag")) != "string" \
		  || trimmed(cur_spec.get("flag")) = ""
			* error('All argumentSpecs must have "flag" string setting')
		}
		? !cur_spec.has("long-flag") \
		  || getType(cur_spec.get("long-flag")) != "string" \
		  || trimmed(cur_spec.get("long-flag")) = ""
			* error('All argumentSpecs must have "long-flag" string setting')
		}
		? !cur_spec.has("description") \
		  || getType(cur_spec.get("description")) != "string" \
		  || trimmed(cur_spec.get("description")) = ""
			* error('All argumentSpecs must have "description" string setting')
		}
		? cur_spec.has("takes") && getType(cur_spec.get("takes")) != "bool"
			* error('argumentSpecs "takes" setting value must be bool')
		}
	}
	
	/ Add default values to the return value
	@ cur_spec : argumentSpecs
		* ret_val.add(cur_spec.get("flag"), null)
	}
	
	/ Validate the EXE version
	? getType(exeVersion) != "string" || exeVersion.length() = 0
		* error("exeVersion must be a non-empty string")
	}

	/ Bail if there are no arguments
	/ This is necessary because if there are no arguments the loop below fails with 0 -> -1
	? arguments.length() = 0
		<- ret_val
	}
	
	/ Loop over the arguments
	# a : 0 -> arguments.length() - 1
		$ cur_arg = arguments.get(a)

		/ Compute the next argument
		$ next_arg = null
		? a < arguments.length() - 1
			& next_arg = arguments.get(a + 1)
		}
		
		/ If it's not a flag, it's raw output
		? cur_arg.get(0) != '-'
			* raw_arg_list.add(cur_arg)
			^
		}
		
		/ Handle -?
		? !already_had_help && (cur_arg = "-?" || cur_arg = "--help")
			@ cur_spec : argumentSpecs
				> fmt("{0} {1}: {2}", \
					  cur_spec.get("flag"), \
					  cur_spec.get("long-flag"), \
					  cur_spec.get("description"))
			}
			* ret_val.set(cur_arg, true)
			^
		}
		
		/ Handle -v
		? !already_had_version && (cur_arg = "-v" || cur_arg = "--version")
			> "mscript version: " + exeVersion
			* ret_val.set(cur_arg, true)
			^
		}
		
		/ Loop over the argument specs to find this argument as a flag or long-flag
		$ found_flag = false
		@ cur_spec : argumentSpecs
			? cur_arg != cur_spec.get("flag") && cur_arg != cur_spec.get("long-flag")
				^
			}
			
			$ cur_flag = cur_spec.get("flag")
			
			? cur_spec.has("takes") and cur_spec.get("takes")
				? next_arg = null
					* error("No value for flag that takes next argument: " + cur_arg)
				}
				<>
					* ret_val.set(cur_flag, next_arg)
					& a = a + 1 // skip the next arg we just processed
				}
			}
			<>
				* ret_val.set(cur_flag, true)
			}
			
			& found_flag = true
			V
		}
		
		? !found_flag
			* error("Unknown command line flag: " + cur_arg)
		}
	}
	
	/ All done
	<- ret_val
}
