#pragma once

#include "pch.h"

namespace mscript
{
	class timestamp
	{
	public:
		static  
			std::string toString
			(
				const uint16_t year, 
				const uint16_t month,
				const uint16_t day,
				const uint16_t hour,
				const uint16_t minute,
				const uint16_t second
			)
		{
			SYSTEMTIME st{};
			st.wYear = year;
			st.wMonth = month;
			st.wDay = day;
			st.wHour = hour;
			st.wMinute = minute;
			st.wSecond = second;
			st.wMilliseconds = 0;
			return sysTimeToString(st);
		}

		static std::string add(const std::string& dt, const char part, const int amount)
		{
			struct tm tm = sysTimeToTm(sysTimeFromString(dt));
			switch (part)
			{
			case 'y': tm.tm_year += amount; break;
			case 'M': tm.tm_mon += amount; break;
			case 'd': tm.tm_mday += amount; break;

			case 'h': tm.tm_hour += amount; break;
			case 'm': tm.tm_min += amount; break;
			case 's': tm.tm_sec += amount; break;

			default:
				std::string c;
				c += part;
				raiseError("Invalid date part to add: " + c);
			}

			SYSTEMTIME st = TimeToSytemTime(mktime(&tm));
			return sysTimeToString(st);
		}

		static int64_t diff(const std::string& dt1, const std::string& dt2, const char outputType)
		{
			struct tm tm1 = sysTimeToTm(sysTimeFromString(dt1));
			time_t t1 = mktime(&tm1);

			struct tm tm2 = sysTimeToTm(sysTimeFromString(dt2));
			time_t t2 = mktime(&tm2);

			int64_t diff = t1 - t2;
			switch (outputType)
			{
			case 'y': return diff / 365 / 86400;
			case 'M': return diff / 30 / 86400;
			case 'd': return diff / 86400;

			case 'h': return diff / 3600;
			case 'm': return diff / 60;
			case 's': return diff;
			
			default:
				std::string c;
				c += outputType;
				raiseError("Invalid date part to output: " + c);
			}
		}

		// https://en.cppreference.com/w/cpp/io/manip/get_time
		// https://en.cppreference.com/w/cpp/io/manip/put_time
		static std::wstring format(const std::string& dt, const std::wstring& fmt)
		{
			SYSTEMTIME st = sysTimeFromString(dt);
			std::tm t{};
			t.tm_year = st.wYear;
			t.tm_mon = st.wMonth;
			t.tm_mday = st.wDay;
			t.tm_hour = st.wHour;
			t.tm_min = st.wMinute;
			t.tm_sec = st.wSecond;

			std::wstringstream out_ss;
			out_ss << std::put_time<wchar_t>(&t, fmt.c_str());
			std::wstring out_str = out_ss.str();
			return out_str;
		}

		static std::string getNow(bool utc)
		{
			SYSTEMTIME st;
			if (utc)
				::GetSystemTime(&st);
			else
				::GetLocalTime(&st);
			return sysTimeToString(st);
		}

		static std::string getFileLastModified(const std::wstring& filePath)
		{
			FILETIME ftCreate, ftAccess, ftWrite;
			GetFileTimes(filePath, ftCreate, ftAccess, ftWrite);
			return fileTimeToString(ftWrite);
		}

		static std::string getFileCreated(const std::wstring& filePath)
		{
			FILETIME ftCreate, ftAccess, ftWrite;
			GetFileTimes(filePath, ftCreate, ftAccess, ftWrite);
			return fileTimeToString(ftCreate);
		}

		static std::string getFileLastAccessed(const std::wstring& filePath)
		{
			FILETIME ftCreate, ftAccess, ftWrite;
			GetFileTimes(filePath, ftCreate, ftAccess, ftWrite);
			return fileTimeToString(ftAccess);
		}

		static std::string toUtc(const std::string& str)
		{
			SYSTEMTIME st = sysTimeFromString(str);
			FILETIME ft{};
			if (!::SystemTimeToFileTime(&st, &ft))
				raiseError("SystemTimeToFileTime failed");
			
			FILETIME localFt{};
			if (!::LocalFileTimeToFileTime(&ft, &localFt))
				raiseError("LocalFileTimeToFileTime failed");

			if (!::FileTimeToSystemTime(&localFt, &st))
				raiseError("FileTimeToSystemTime failed");
			
			return sysTimeToString(st);
		}

		static std::string toLocal(const std::string& str)
		{
			SYSTEMTIME st = sysTimeFromString(str);
			FILETIME ft{};
			if (!::SystemTimeToFileTime(&st, &ft))
				raiseError("SystemTimeToFileTime failed");

			FILETIME localFt{};
			if (!::FileTimeToLocalFileTime(&ft, &localFt))
				raiseError("FileTimeToLocalFileTime failed");

			if (!::FileTimeToSystemTime(&localFt, &st))
				raiseError("FileTimeToSystemTime failed");

			return sysTimeToString(st);
		}

		static void touch(const std::wstring& filePath, const std::string& timestamp)
		{
			SYSTEMTIME st;
			if (timestamp.empty())
				::GetSystemTime(&st);
			else
				st = sysTimeFromString(timestamp);

			FILETIME ft{};
			if (!::SystemTimeToFileTime(&st, &ft))
				raiseError("SystemTimeToFileTime failed");

			HANDLE hFile =
				::CreateFile
				(
					filePath.c_str(),
					FILE_WRITE_ATTRIBUTES,
					FILE_SHARE_READ,
					nullptr,
					OPEN_EXISTING,
					0,
					nullptr
				);
			if (hFile == nullptr)
				raiseError("CreateFile failed");

			BOOL success = ::SetFileTime(hFile, nullptr, nullptr, &ft);
			::CloseHandle(hFile);
			if (!success)
				raiseError("SetFileTime failed");
		}

		static char convertPartStr(const std::string& part)
		{
			char partChar = '\0';
			if (part == "year")
				partChar = 'y';
			else if (part == "month")
				partChar = 'M';
			else if (part == "day")
				partChar = 'd';
			else if (part == "hour")
				partChar = 'h';
			else if (part == "minute")
				partChar = 'm';
			else if (part == "second")
				partChar = 's';
			else
				raiseError("Invalid date part: " + part);
			return partChar;
		}

	private:
		static void GetFileTimes(const std::wstring& filePath, FILETIME& ftCreate, FILETIME& ftAccess, FILETIME& ftWrite)
		{
			HANDLE hFile =
				::CreateFile
				(
					filePath.c_str(),
					GENERIC_READ,
					FILE_SHARE_READ,
					nullptr,
					OPEN_EXISTING,
					0,
					nullptr
				);
			if (hFile == nullptr)
				raiseError("CreateFile failed");

			BOOL success = ::GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
			::CloseHandle(hFile);
			if (!success)
				raiseError("GetFileTime failed");
		}

		static std::string fileTimeToString(const FILETIME& ft)
		{
			SYSTEMTIME st{};
			if (!FileTimeToSystemTime(&ft, &st))
				raiseError("FileTimeToSystemTime failed");
			else
				return sysTimeToString(st);
		}

		static std::string sysTimeToString(const SYSTEMTIME& sysTime)
		{
			std::stringstream msg;
			msg << sysTime.wYear 
				<< "-" << std::setw(2) << std::setfill('0') << sysTime.wMonth 
				<< "-" << std::setw(2) << std::setfill('0') << sysTime.wDay 
				<< " "
				<< std::setw(2) << std::setfill('0') << sysTime.wHour 
				<< ":" << std::setw(2) << std::setfill('0') << sysTime.wMinute 
				<< ":" << std::setw(2) << std::setfill('0') << sysTime.wSecond;
			return msg.str();
		}

		static SYSTEMTIME sysTimeFromString(const std::string& stringToParse)
		{
			SYSTEMTIME st{};
			sscanf_s
			(
				stringToParse.c_str(), 
				"%hd-%hd-%hd %hd:%hd:%hd",
				&st.wYear,
				&st.wMonth,
				&st.wDay,
				&st.wHour,
				&st.wMinute,
				&st.wSecond
			);
			return st;
		}

		static tm sysTimeToTm(const SYSTEMTIME& st)
		{
			struct tm tm {};
			tm.tm_year = st.wYear - 1900;
			tm.tm_mon = st.wMonth;
			tm.tm_mday = st.wDay;
			tm.tm_hour = st.wHour;
			tm.tm_min = st.wMinute;
			tm.tm_sec = st.wSecond;
			return tm;
		}

		// https://docs.microsoft.com/en-us/windows/win32/sysinfo/converting-a-time-t-value-to-a-file-time
		static SYSTEMTIME TimeToSytemTime(time_t t)
		{
			FILETIME ft{};
			ULARGE_INTEGER time_value;
			time_value.QuadPart = (t * 10000000LL) + 116444736000000000LL;
			ft.dwLowDateTime = time_value.LowPart;
			ft.dwHighDateTime = time_value.HighPart;
			
			SYSTEMTIME st{};
			if (!::FileTimeToSystemTime(&ft, &st))
				raiseError("FileTimeToSystemTime failed");
			return st;
		}
	};
}
