#pragma once

#include <Windows.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace mscript
{
	class datetime
	{
	public:
		// FORNOW
		// getDatePart(str, part)
		// getDateDiff(units, big, little)
		// addToDate(units, amount, str)

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
			if (!GetFileTimes(filePath, ftCreate, ftAccess, ftWrite))
				return "";
			else
				return fileTimeToString(ftWrite);
		}

		static std::string getFileCreated(const std::wstring& filePath)
		{
			FILETIME ftCreate, ftAccess, ftWrite;
			if (!GetFileTimes(filePath, ftCreate, ftAccess, ftWrite))
				return "";
			else
				return fileTimeToString(ftCreate);
		}

		static std::string getFileLastAccessed(const std::wstring& filePath)
		{
			FILETIME ftCreate, ftAccess, ftWrite;
			if (!GetFileTimes(filePath, ftCreate, ftAccess, ftWrite))
				return "";
			else
				return fileTimeToString(ftAccess);
		}

		static std::string toUtc(const std::string& str)
		{
			SYSTEMTIME st = sysTimeFromString(str);
			FILETIME ft{};
			if (!::SystemTimeToFileTime(&st, &ft))
				return "";

			FILETIME localFt{};
			if (!::LocalFileTimeToFileTime(&ft, &localFt))
				return "";

			if (!::FileTimeToSystemTime(&localFt, &st))
				return "";

			return sysTimeToString(st);
		}

		static std::string toLocal(const std::string& str)
		{
			SYSTEMTIME st = sysTimeFromString(str);
			FILETIME ft{};
			if (!::SystemTimeToFileTime(&st, &ft))
				return "";

			FILETIME localFt{};
			if (!::FileTimeToLocalFileTime(&ft, &localFt))
				return "";

			if (!::FileTimeToSystemTime(&localFt, &st))
				return "";

			return sysTimeToString(st);
		}

		BOOL touch(const std::wstring& filePath)
		{
			SYSTEMTIME st;
			::GetSystemTime(&st);
			FILETIME ft{};
			if (!::SystemTimeToFileTime(&st, &ft))
				return FALSE;

			HANDLE hFile =
				::CreateFile
				(
					filePath.c_str(),
					GENERIC_WRITE,
					FILE_SHARE_READ,
					nullptr,
					OPEN_EXISTING,
					0,
					nullptr
				);
			if (hFile == nullptr)
				return FALSE;

			BOOL success = ::SetFileTime(hFile, nullptr, nullptr, &ft);
			::CloseHandle(hFile);
			return success;
		}

		static BOOL GetFileTimes(const std::wstring& filePath, FILETIME& ftCreate, FILETIME& ftAccess, FILETIME& ftWrite)
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
				return FALSE;

			BOOL success = ::GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
			::CloseHandle(hFile);
			return success;
		}

		static std::string fileTimeToString(const FILETIME& ft)
		{
			SYSTEMTIME st{};
			if (!FileTimeToSystemTime(&ft, &st))
				return "";
			else
				return sysTimeToString(st);
		}

		static std::string sysTimeToString(const SYSTEMTIME& sysTime)
		{
			std::stringstream msg;
			msg << sysTime.wYear << "-"
				<< std::setw(2) << std::setfill('0') << sysTime.wMonth << "-"
				<< std::setw(2) << std::setfill('0') << sysTime.wDay << " "
				<< std::setw(2) << std::setfill('0') << sysTime.wHour << ":"
				<< std::setw(2) << std::setfill('0') << sysTime.wMinute << ":"
				<< std::setw(2) << std::setfill('0') << sysTime.wSecond << "."
				<< std::setw(3) << std::setfill('0') << sysTime.wMilliseconds;
			return msg.str();
		}

		static SYSTEMTIME sysTimeFromString(const std::string& stringToParse)
		{
			SYSTEMTIME st{};
			sscanf_s
			(
				stringToParse.c_str(), 
				"%d-%d-%d %d:%d:%d.%d",
				&st.wYear,
				&st.wMonth,
				&st.wDay,
				&st.wHour,
				&st.wMinute,
				&st.wMilliseconds
			);
			return st;
		}
	};
}