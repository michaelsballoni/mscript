#include "pch.h"
#include "cx_Logging.h"

using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"mslog_start",
		L"mslog_stop",
		L"mslog_log",
		L"mslog_trace",
	};
	return module_utils::getExports(exports);
}

void mscript_FreeString(wchar_t* str)
{
	delete[] str;
}

wchar_t* mscript_ExecuteFunction(const wchar_t* functionName, const wchar_t* parametersJson)
{
	try
	{
		std::wstring funcName = functionName;
		auto params = module_utils::getParams(parametersJson);

		if (funcName == L"mslog_stop")
		{
			StopLogging();
			return module_utils::jsonStr(object());
		}

		if (funcName == L"mslog_start")
		{
			if (params.size() != 1)
				raiseError("mslog_start: Index parameter not passed in");;

			if (params[0].type() != object::INDEX)
				raiseError("mslog_start: Parameter passed in is not an index");

			const object::index& index = params[0].indexVal();

			object filename;
			if
			(
				!index.tryGet(toWideStr("filename"), filename) 
				|| 
				filename.type() != object::STRING
			)
			{
				raiseError("mslog_start: filename value not passed in");
			}

			object level;
			if
			(
				!index.tryGet(toWideStr("level"), level)
				||
				level.type() != object::NUMBER
				||
				level.numberVal() < 0
				||
				unsigned(level.numberVal()) != level.numberVal()
			)
			{
				raiseError("mslog_start: invalid level parameter");
			}

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
				raiseError("mslog_start: invalid maxFiles parameter");
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
				raiseError("mslog_start: invalid maxFileSizeBytes parameter");
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
				raiseError("mslog_start: invalid prefix parameter");
			}
			if (prefix.type() == object::NOTHING)
				prefix = toWideStr(DEFAULT_PREFIX);

			StartLogging
			(
				toNarrowStr(filename.stringVal()).c_str(),
				int(level.numberVal()),
				unsigned(max_files.numberVal()),
				unsigned(max_file_size_bytes.numberVal()),
				toNarrowStr(prefix.stringVal()).c_str()
			);
		}

		raiseWError(L"Unknown mscript-dll-sample function: " + funcName);
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
