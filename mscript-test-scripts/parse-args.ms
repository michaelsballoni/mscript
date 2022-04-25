/*
mscript prototype of CLI argument processing
Takes the list of arguments from wmain,
and specifications about how to process the arguments
Returns an index matching flag names to processed values
and the special value "raw" which is a list of all un-tagged arguments
*/
~ parseArgs(arguments, argumentSpecs)
	$ ret_val = index()
	
	/ Start with a blank list of un-flagged arguments
	ret_val.add("raw", list())
	
	/ Turn the given list of spec indexes into an index mapping tag to index
	$ spec_index = index()
	@ arg_spec : argumentSpecs
		$ cur_tag = arg_spec.get("flag")
		? spec_index.has(cur_tag)
			* error("Duplicate argument specification flag: " + cur_tag)
		}
		* spec_index.set(cur_tag, arg_spec)
	}
	
	# a : 0 -> arguments.length() - 1
		$ cur_arg = arguments.get(a)
		
		$ next_arg = null
		? a < arguments.length() - 1
			next_arg = arguments.get(a + 1)
		}
		
		? cur_arg.get(0) = '-'
			/ Implement hardcoded help flags
			? cur_arg = "-?" || cur_arg = "--help"
				> "-? or --help: output this command-line usage"
				@ arg_key : spec_index
					$ cur_spec = spec_index.get(arg_key)
					> fmt("{0}: {1}", cur_spec.get("tag"), cur_spec.get("description"))
				}
				^
			}
			
			? not format_index.has(cur_arg)
				* error("Unexpected command line flag: " + cur_arg)
			}
			
			$ cur_spec = spec_index.get(cur_arg)
			? cur_spec.has("takes") and fmt.get("takes")
				? next_arg = null
					* error("No value provided for tag that processes a next tag: " + cur_arg)
				}
				? next_arg.length() > 0 and next_arg.get(0) = '-'
					* error(fmt("Argument after flag is another flag: {0} - {1}", cur_arg, next_arg)
				}
				<>
					ret_val.set(cur_arg, next_arg)
					a = a + 1 // skip the next arg we just processed
				}
			}
		}
		<>
			* add(ret_val.get("raw"), cur_arg)
		}
	}
	<- ret_val
}
