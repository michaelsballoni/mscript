#include "pch.h"

#include "mscript-timestamp.h"

using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"msts_build",
		L"msts_add",
		L"msts_diff",
		L"msts_format",

		L"msts_now",

		L"msts_last_modified",
		L"msts_created",
		L"msts_last_accessed",

		L"msts_to_utc",
		L"msts_to_local",

		L"msts_touch",
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

		if (funcName == L"msts_build")
		{
			auto numParams = module_utils::getNumberParams(parametersJson);
			if (numParams.size() == 3)
			{
				std::string str =
					timestamp::toString
					(
						uint16_t(numParams[0]),
						uint16_t(numParams[1]),
						uint16_t(numParams[2]),
						0,
						0,
						0
					);
				return module_utils::jsonStr(str);
			}
			else if (numParams.size() == 6)
			{
				std::string str =
					timestamp::toString
					(
						uint16_t(numParams[0]),
						uint16_t(numParams[1]),
						uint16_t(numParams[2]),
						uint16_t(numParams[3]),
						uint16_t(numParams[4]),
						uint16_t(numParams[5])
					);
				return module_utils::jsonStr(str);
			}
			else
				raiseError("msts_to_string takes either three date parameters or six date-time parameters");
		}

		auto params = module_utils::getParams(parametersJson);

		if (funcName == L"msts_add")
		{
			if 
			(
				params.size() != 3 
				|| 
				params[0].type() != object::STRING
				||
				params[1].type() != object::STRING
				||
				params[2].type() != object::NUMBER
			)
			{
				raiseError("msts_add takes three parameters: timestamp string, part to add to string, and amount to add number");
			}

			std::string ts = toNarrowStr(params[0].stringVal());
			std::string part = toNarrowStr(toLower(params[1].stringVal()));
			int amount = int(params[2].numberVal());

			char partChar = timestamp::convertPartStr(part);
			std::string addedTs = timestamp::add(ts, partChar, amount);
			return module_utils::jsonStr(addedTs);
		}

		if (funcName == L"msts_diff")
		{
			if
			(
				params.size() != 3
				||
				params[0].type() != object::STRING
				||
				params[1].type() != object::STRING
				||
				params[2].type() != object::STRING
			)
			{
				raiseError("msts_diff takes three parameters: two timestamp strings, and the part to add to diff");
			}

			std::string ts1 = toNarrowStr(params[0].stringVal());
			std::string ts2 = toNarrowStr(params[1].stringVal());
			std::string part = toNarrowStr(toLower(params[2].stringVal()));

			char partChar = timestamp::convertPartStr(part);
			auto diffAmount = timestamp::diff(ts1, ts2, partChar);
			return module_utils::jsonStr(double(diffAmount));
		}

		if (funcName == L"msts_format")
		{
			if
			(
				params.size() != 2
				||
				params[0].type() != object::STRING
				||
				params[1].type() != object::STRING
			)
			{
				raiseError("msts_format takes two parameters: timestamp string, format string");
			}

			std::wstring ts = params[0].stringVal();
			std::wstring format = params[1].stringVal();

			std::wstring str = timestamp::format(toNarrowStr(ts), format);
			return module_utils::jsonStr(str);
		}

		if (funcName == L"msts_now")
		{
			bool useUtc = true;
			if (params.size() == 1)
			{
				if (params[0].type() != object::BOOL)
					raiseWError(L"msts_now can take one boolean parameter, is_utc");
				useUtc = params[0].boolVal();
			}
			else if (params.size() != 0)
				raiseWError(L"msts_now can take at most one boolean parameter, is_utc");
			std::string now = timestamp::getNow(useUtc);
			return module_utils::jsonStr(now);
		}

		if (funcName == L"msts_touch")
		{
			std::wstring filePath;
			std::wstring timestamp;
			if (params.size() == 1)
			{
				if (params[0].type() != object::STRING)
					raiseWError(L"msts_touch can take one string parameter, file_path");
				filePath = params[0].stringVal();
			}
			else if (params.size() == 2)
			{
				if (params[0].type() != object::STRING)
					raiseWError(L"msts_touch can take one string parameter, file_path");
				if (params[1].type() != object::STRING)
					raiseWError(L"msts_touch can take a second string parameter, timestamp");
				filePath = params[0].stringVal();
				timestamp = params[1].stringVal();
			}
			else
				raiseWError(L"msts_touch can take one file_path parameter, and an optional timetamp parameter");

			timestamp::touch(filePath, toNarrowStr(timestamp));
			return module_utils::jsonStr(true);
		}

		std::wstring strParam;
		if (params.size() == 1)
		{
			strParam = params[0].stringVal();
		}
		else
			raiseWError(L"Function " + funcName + L" takes one string parameter");

		if (funcName == L"msts_last_modified")
		{
			std::string date = timestamp::getFileLastModified(strParam);
			return module_utils::jsonStr(date);
		}
		
		if (funcName == L"msts_created")
		{
			std::string date = timestamp::getFileLastModified(strParam);
			return module_utils::jsonStr(date);
		}
		
		if (funcName == L"msts_last_accessed")
		{
			std::string date = timestamp::getFileLastModified(strParam);
			return module_utils::jsonStr(date);
		}
		
		if (funcName == L"msts_to_utc")
		{
			std::string date = timestamp::toUtc(toNarrowStr(strParam));
			return module_utils::jsonStr(date);
		}
		
		if (funcName == L"msts_to_local")
		{
			std::string date = timestamp::toLocal(toNarrowStr(strParam));
			return module_utils::jsonStr(date);
		}

		raiseWError(L"Unknown timestamp function: " + funcName);
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
