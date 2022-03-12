#pragma once

#include "../mscript-core/utils.h"

#include <Windows.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace mscript
{
	class timestamp
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

		static void touch(const std::wstring& filePath)
		{
			SYSTEMTIME st;
			::GetSystemTime(&st);
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
	};
}
