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
	
	$ raw_arg_list = list()
	* ret_val.add("", raw_arg_list)
	
	? getType(arguments) != "list"
		* error("arguments must be a list")
	}
	@ cur_arg : arguments
		? getType(cur_arg) != "string"
			* error("All arguments must be strings")
		}
	}
	
	@ cur_spec : argumentSpecs
		? getType(cur_spec) != "index"
			* error("All argumentSpecs must be indexes")
		}
		? !cur_spec.has("flag") \
		  || getType(cur_spec.get("flag")) != "string" \
		  || trim(cur_spec.get("flag")) = ""
			* error('All argumentSpecs must have "flag" string setting')
		}
		? !cur_spec.has("long-flag") \
		  || getType(cur_spec.get("long-flag")) != "string" \
		  || trim(cur_spec.get("long-flag")) = ""
			* error('All argumentSpecs must have "long-flag" string setting')
		}
		? !cur_spec.has("description") \
		  || getType(cur_spec.get("description")) != "string" \
		  || trim(cur_spec.get("description")) = ""
			* error('All argumentSpecs must have "description" string setting')
		}
		? !cur_spec.has("takes") \
		  || getType(cur_spec.get("takes")) != "bool"
			* error('argumentSpecs "take" setting value must be bool')
		}
	}
	
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
								  "description", "Get the usage info about this script", \
								  "takes", false))
	}
							  
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
								  "description", "Get the version of the mscript EXE running this script", \
								  "takes", false))
	}
	
	@ arg_spec : argumentSpecs
		* ret_val.set(arg_spec.get("flag"), false)
	}
	
	? getType(exeVersion) != "string" || exeVersion.length() = 0
		* error("exeVersion must be a non-empty string")
	}
	
	? arguments.length() = 0
		<- ret_val
	}
	
	# a : 0 -> arguments.length() - 1
		$ cur_arg = arguments.get(a)
		
		$ next_arg = null
		? a < arguments.length() - 1
			& next_arg = arguments.get(a + 1)
		}
		
		? cur_arg.get(0) = '-'
			? !already_had_help && (cur_arg = "-?" || cur_arg = "--help")
				> "-? or --help: output this command-line usage"
				@ arg_key : spec_index
					$ cur_spec = spec_index.get(arg_key)
					> fmt("{0} - {1}: {2}", \
						  cur_spec.get("flag"), \
						  cur_spec.get("long-flag"), \
						  cur_spec.get("description"))
				}
			}
			? !already_had_version && (cur_arg = "-v" || cur_arg = "--version")
				> "mscript version: " + exeVersion
			}
			<>
				$ found_spec = false
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
					& found_spec = true
					V
				}
			}
			? !found_spec
				* error(fmt("Unknown command-line flag: {0}", cur_arg))
			}
		}
		<>
			* raw_arg_list.add(cur_arg)
		}
	}
	<- ret_val
}
