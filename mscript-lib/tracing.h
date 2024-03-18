#pragma once

#include <object.h>

#include <string>
#include <vector>

namespace mscript
{
	enum TraceLevel
	{
		TRACE_NONE = 100,
		TRACE_CRITICAL = 50,
		TRACE_ERROR = 40,
		TRACE_WARNING = 30,
		TRACE_INFO = 20,
		TRACE_DEBUG = 10
	};

	struct tracing
	{
		std::vector<std::wstring> ActiveSections;
		TraceLevel CurrentTraceLevel = TRACE_NONE;

		bool DoesSectionMatch(const std::wstring& section)
		{
			return std::find(ActiveSections.begin(), ActiveSections.end(), section) != ActiveSections.end();
		}

		bool DoesLevelMatch(TraceLevel level)
		{
			return CurrentTraceLevel <= level;
		}
	};
}
