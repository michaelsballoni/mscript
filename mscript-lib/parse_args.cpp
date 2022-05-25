#include "pch.h"
#include "parse_args.h"
#include "exe_version.h"
#include "utils.h"

using namespace mscript;

struct arg_spec
{
	// required flag data
	std::wstring flag; // -?
	std::wstring long_flag; // --help
	std::wstring description; // Get some help!

	// optional flags
	bool takes = false;
	bool numeric = false;
	bool required = false;

	object default_value = object(false);
};

static std::wstring getStrFlagValue(const object::index& argumentSpec, const std::string& flagName)
{
	object flag_value;
	if (!argumentSpec.tryGet(toWideStr(flagName), flag_value) || flag_value.type() != object::STRING)
		raiseError("Argument spec lacks " + flagName + " string setting");

	std::wstring flag_str = trim(flag_value.stringVal());
	if (flag_str.empty())
		raiseError("Argument spec lacks " + flagName + " non-empty string setting");

	return flag_str;
}

static bool getBoolFlagValue(const object::index& argumentSpec, const std::string& flagName)
{
	object flag_value;
	if (!argumentSpec.tryGet(toWideStr(flagName), flag_value))
		return false;

	if (flag_value.type() != object::BOOL)
		raiseError("Argument spec " + flagName + " setting is not bool");

	return flag_value.boolVal();
}

object
mscript::parseArgs
(
	const object::list& arguments,
	const object::list& argumentSpecs
)
{
	// Set up shop
	object ret_val = object::index();
	object::index& ret_val_index = ret_val.indexVal();

	// Start off with our list of un-flagged arguments
	// NOTE: The object::list inside raw_arg_list is passed by reference
	//		 so changes made to it along the way are reflected in the returned list
	object raw_arg_list = object::list();
	ret_val_index.set(toWideStr(""), raw_arg_list);

	// Pre-process the argument specs
	std::vector<arg_spec> local_specs;
	for (const auto& cur_input_spec : argumentSpecs)
	{
		if (cur_input_spec.type() != object::INDEX)
			raiseError("Invalid argument spec type: " + object::getTypeName(cur_input_spec.type()));
		const object::index& cur_input_index = cur_input_spec.indexVal();

		arg_spec cur_spec;
		cur_spec.flag = getStrFlagValue(cur_input_index, "flag");
		cur_spec.long_flag = getStrFlagValue(cur_input_index, "long-flag");
		cur_spec.description = getStrFlagValue(cur_input_index, "description");
		cur_spec.takes = getBoolFlagValue(cur_input_index, "takes");
		cur_spec.numeric = getBoolFlagValue(cur_input_index, "numeric");
		cur_spec.required = getBoolFlagValue(cur_input_index, "required");

		object default_value;
		if (cur_input_index.tryGet(toWideStr("default"), default_value))
			cur_spec.default_value = default_value;
		local_specs.push_back(cur_spec);
	}

	// Add help flag processing if not already defined
	bool already_had_help = false;
	for (const auto& cur_spec : local_specs)
	{
		if (cur_spec.flag == L"-?" || cur_spec.long_flag == L"--help")
		{
			already_had_help = true;
			break;
		}
	}
	if (!already_had_help)
	{
		arg_spec new_spec;
		new_spec.flag = L"-?";
		new_spec.long_flag = L"--help";
		new_spec.description = L"Get usage of this script";
		local_specs.insert(local_specs.begin(), new_spec);
	}

	// Add default values (nulls) to the return value index
	for (const auto& arg_spec : local_specs)
		ret_val_index.set(arg_spec.flag, arg_spec.default_value);

	// Validate the arguments
    bool help_exit_suppressed = false;
	for (size_t a = 0; a < arguments.size(); ++a)
	{
		if (arguments[a].type() != object::STRING)
		{
			raiseWError(L"Invalid command-line argument, not a string: #" + num2wstr(double(a)) +
						L" - " + arguments[a].toString());
		}
		if (arguments[a].stringVal() == L"--suppress-help-quit")
			help_exit_suppressed = true;
	}

	// Loop over the arguments
	bool help_was_output = false;
	for (size_t a = 0; a < arguments.size(); ++a)
	{
		const std::wstring& cur_arg = arguments[a].stringVal();
		if (cur_arg == L"--suppress-help-quit")
			continue;

		bool has_next_arg = false;
		object next_arg;
		if (a < arguments.size() - 1)
		{
			next_arg = arguments[a + 1];
			if (!startsWith(next_arg.toString(), L"-"))
				has_next_arg = true;
		}

		if (cur_arg.empty() || cur_arg[0] != '-')
		{
			raw_arg_list.listVal().push_back(cur_arg);
			continue;
		}

		if (!already_had_help && (cur_arg == L"-?" || cur_arg == L"--help"))
		{
			std::wstring mscript_exe_path = getExeFilePath();
			std::wcout
				<< mscript_exe_path
				<< L" - v" << toWideStr(getBinaryVersion(mscript_exe_path))
				<< L"\n";

			size_t max_flags_len = 0;
			for (const auto& arg_spec : local_specs)
			{
				size_t cur_len = arg_spec.flag.size() + arg_spec.long_flag.size();
				if (cur_len > max_flags_len)
					max_flags_len = cur_len;
			}
			static std::wstring flag_separator = L", ";
			static size_t flag_separator_len = flag_separator.length();
			size_t max_flags_output_len = max_flags_len + flag_separator_len;
			for (const auto& arg_spec : local_specs)
			{
				std::wcout
					<< std::left
					<< std::setfill(L' ')
					<< std::setw(max_flags_output_len)
					<< (arg_spec.flag + flag_separator + arg_spec.long_flag)
					<< L": "
					<< arg_spec.description;

				if (arg_spec.takes)
				{
					std::wcout << " - type=" << (arg_spec.numeric ? "num" : "str");
					
					if (arg_spec.default_value.type() != object::NOTHING)
						std::wcout << " - default=" << arg_spec.default_value.toString();

					if (arg_spec.required)
						std::wcout << " - [REQUIRED]";
				}

				std::wcout << L"\n";
			}

			std::wcout << std::flush;

			if (!help_exit_suppressed)
				exit(0);

			ret_val_index.set(toWideStr("-?"), true);
			help_was_output = true;
			continue;
		}

		// Loop over the argument specs to find this argument as a flag or long flag
		bool found_flag = false;
		for (const auto& cur_spec : local_specs)
		{
			if (!(cur_spec.flag == cur_arg || cur_spec.long_flag == cur_arg))
				continue;
			else
				found_flag = true;

			const std::wstring& cur_flag = cur_spec.flag;
			if (cur_spec.takes)
			{
				if (!has_next_arg)
				{
					if (cur_spec.default_value != object())
						next_arg = cur_spec.default_value;
					else
						raiseWError(L"No value for flag that takes next argument: " + cur_arg);
				}
				
				// Convert the next argument into its final value
				if (cur_spec.numeric && next_arg.type() != object::NUMBER)
				{
					try
					{
						ret_val_index.set(cur_flag, std::stod(next_arg.toString()));
					}
					catch (...)
					{
						raiseWError(L"Converting argument to number failed: " +
									cur_spec.flag + L" - " + next_arg.toString());
					}
				}
				else
					ret_val_index.set(cur_flag, next_arg);

				// Skip the next argument we just processed
				a = a + 1; 
			}
			else // non-taking flag
			{
				ret_val_index.set(cur_flag, true);
			}
		}

		if (!found_flag)
			raiseWError(L"Unknown command line flag: " + cur_arg);
	}

	// Enforce that all required flags were specified...unless -? / --help
	if (!help_was_output)
	{
		for (const auto& cur_spec : local_specs)
		{
			if (cur_spec.required && ret_val_index.get(cur_spec.flag).type() == object::NOTHING)
				raiseWError(L"Required argument not provided: " + cur_spec.flag);
		}
	}

	// All done
	return ret_val;
}
