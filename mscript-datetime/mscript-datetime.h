#pragma once

#include <Windows.h>

#include <string>

namespace mscript
{
	class datetime
	{
	public:
		datetime(const std::wstring& stringToParse, bool utc = true);
		// https://en.cppreference.com/w/cpp/io/manip/get_time

		datetime(SYSTEMTIME systemTime, bool utc = true);
		datetime(FILETIME fileTime, bool utc = true);
		datetime(tm* timeStruct, bool utc = true);

		std::wstring format(const std::wstring& formatString);
		/*
		std::stringstream buffer;
		buffer << std::put_time(&your_tm, "%a %b %d %H:%M:%S %Y");
		*/

		datetime getNow(bool utc = true);

		datetime getFileLastModified(const std::wstring& filePath);
		datetime getFileCreated(const std::wstring& filePath);

		void toUtc();
		void toLocal();

		void touch(const std::wstring& filePath);

	private:
		bool m_isUtc;
		SYSTEMTIME m_sysTime;
	};
}