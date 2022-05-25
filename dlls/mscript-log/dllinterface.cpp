#include "pch.h"
#include "cx_Logging.h"

using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"mslog_getlevel",
		L"mslog_setlevel",

		L"mslog_start",
		L"mslog_stop",
		
		L"mslog_error",
		L"mslog_info",
		L"mslog_debug",
	};
	return module_utils::getExports(exports);
}

void mscript_FreeString(wchar_t* str)
{
	delete[] str;
}

static unsigned long getLogLevel(const std::wstring& level)
{
	if (level == L"INFO")
		return LOG_LEVEL_INFO;
	else if (level == L"DEBUG")
		return LOG_LEVEL_DEBUG;
	else if (level == L"ERROR")
		return LOG_LEVEL_ERROR;
	else if (level == L"NONE")
		return LOG_LEVEL_NONE;
	else
		raiseWError(L"Invalid log level: " + level);
}

static std::wstring getLogLevel(unsigned long level)
{
	switch (level)
	{
	case LOG_LEVEL_INFO: return L"INFO";
	case LOG_LEVEL_DEBUG: return L"DEBUG";
	case LOG_LEVEL_ERROR: return L"ERROR";
	case LOG_LEVEL_NONE: return L"NONE";
	default: raiseError("Invalid log level: " + num2str(level));
	}
}

wchar_t* mscript_ExecuteFunction(const wchar_t* functionName, const wchar_t* parametersJson)
{
	try
	{
		std::wstring funcName = functionName;
		auto params = module_utils::getParams(parametersJson);

		if (funcName == L"mslog_getlevel")
		{
			if (params.size() != 0)
				raiseError("Takes no parameters");
			return module_utils::jsonStr(getLogLevel(GetLoggingLevel()));
		}

		if (funcName == L"mslog_setlevel")
		{
			if (params.size() != 1 || params[0].type() != object::STRING)
				raiseError("Pass in the log level: DEBUG, INFO, ERROR, or NONE");
			unsigned result = SetLoggingLevel(getLogLevel(toUpper(params[0].stringVal())));
			return module_utils::jsonStr(bool(result == 0));
		}

		if (funcName == L"mslog_start")
		{
			if (params.size() < 2 || params.size() > 3)
				raiseError("Takes three parameters: filename, log level, and optional settings index");;

			if (params[0].type() != object::STRING)
				raiseError("First parameter must be filename string");
			if (params[1].type() != object::STRING)
				raiseError("Second parameter must be log level string: DEBUG, INFO, ERROR, or NONE");
			if (params.size() == 3 && params[2].type() != object::INDEX)
				raiseError("Third parameter must be a settings index");

			std::wstring filename = params[0].stringVal();
			unsigned log_level = getLogLevel(toUpper(params[1].stringVal()));

			object::index index = 
				params.size() == 3 
				? params[2].indexVal() 
				: object::index();

			object max_files;
			if
			(
				index.tryGet(toWideStr("maxFiles"), max_files)
				&&
				(
					max_files.type() != object::NUMBER
					||
					max_files.numberVal() < 0
					||
					unsigned(max_files.numberVal()) != max_files.numberVal()
				)
			)
			{
				raiseError("Invalid maxFiles parameter");
			}
			if (max_files.type() == object::NOTHING)
				max_files = double(1);

			object max_file_size_bytes;
			if
			(
				index.tryGet(toWideStr("maxFileSizeBytes"), max_file_size_bytes)
				&&
				(
					max_file_size_bytes.type() != object::NUMBER
					||
					max_file_size_bytes.numberVal() < 0
					||
					unsigned(max_file_size_bytes.numberVal()) != max_file_size_bytes.numberVal()
				)
			)
			{
				raiseError("Invalid maxFileSizeBytes parameter");
			}
			if (max_file_size_bytes.type() == object::NOTHING)
				max_file_size_bytes = double(DEFAULT_MAX_FILE_SIZE);

			object prefix;
			if
			(
				index.tryGet(toWideStr("prefix"), prefix)
				&&
				prefix.type() != object::STRING
			)
			{
				raiseError("Invalid prefix parameter");
			}
			if (prefix.type() == object::NOTHING)
				prefix = toWideStr(DEFAULT_PREFIX);

			unsigned result = 
				StartLogging
				(
					toNarrowStr(filename).c_str(),
					log_level,
					unsigned(max_files.numberVal()),
					unsigned(max_file_size_bytes.numberVal()),
					toNarrowStr(prefix.stringVal()).c_str()
				);
			return module_utils::jsonStr(bool(result == 0));
		}

		if (funcName == L"mslog_stop")
		{
			StopLogging();
			return module_utils::jsonStr(true);
		}

		{
			auto under_parts = split(funcName, L"_");
			if (under_parts.size() != 2)
				raiseWError(L"Invalid function name: " + funcName);

			unsigned int log_level = getLogLevel(toUpper(under_parts[1]));

			if (params.size() != 1 || params[0].type() != object::STRING)
				raiseWError(funcName + L": pass the log message as one string parameter");

			unsigned result =
				LogMessage(log_level, toNarrowStr(params[0].stringVal()).c_str());
			return module_utils::jsonStr(bool(result == 0));
		}

		// Unreachable
		//raiseWError(L"Unknown mscript-log function: " + funcName);
	}
	catch (const user_exception& exp)
	{
		return module_utils::errorStr(functionName, exp);
	}
	catch (const std::exception& exp)
	{
		return module_utils::errorStr(functionName, exp);
	}
	catch (...)
	{
		return nullptr;
	}
}
