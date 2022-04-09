#pragma once
#include "pch.h"

using namespace mscript;

class registry
{
public:
	registry(const object::list& params)
		: m_params(params)
		, m_root_key(nullptr)
		, m_local_key(nullptr)
	{
		if (m_params.empty() || m_params[0].type() != object::STRING)
			raiseError("Registry functions take a first registry key string parameter");
		m_input_path = m_params[0].stringVal();

		size_t first_slash = m_input_path.find('\\');
		if (first_slash == std::wstring::npos)
			raiseWError(L"Invalid registry key (no slash): " + m_input_path);

		m_path = m_input_path.substr(first_slash + 1);
		if (m_path.empty())
			raiseWError(L"Invalid registry key (empty key): " + m_input_path);

		std::wstring reg_root;
		reg_root = toUpper(m_input_path.substr(0, first_slash));
		if (reg_root.empty())
			raiseWError(L"Invalid registry key (invalid m_root_key): " + m_input_path);

		if (reg_root == L"HKCR" || reg_root == L"HKEY_CLASSES_ROOT")
			m_root_key = HKEY_CLASSES_ROOT;
		else if (reg_root == L"HKCC" || reg_root == L"HKEY_CURRENT_CONFIG")
			m_root_key = HKEY_CURRENT_CONFIG;
		else if (reg_root == L"HKCU" || reg_root == L"HKEY_CURRENT_USER")
			m_root_key = HKEY_CURRENT_USER;
		else if (reg_root == L"HKLM" || reg_root == L"HKEY_LOCAL_MACHINE")
			m_root_key = HKEY_LOCAL_MACHINE;
		else if (reg_root == L"HKU" || reg_root == L"HKEY_USERS")
			m_root_key = HKEY_USERS;
		else
			raiseWError(L"Invalid registry key (unknown root): " + m_input_path + L" (" + reg_root + L")");
	}

	~registry()
	{
		if (m_local_key != nullptr)
			::RegCloseKey(m_local_key);
	}

	void createKey()
	{
		DWORD dwError = ::RegCreateKey(m_root_key, m_path.c_str(), &m_local_key);
		if (dwError != ERROR_SUCCESS)
			raiseWError(L"Creating key failed: " + m_input_path + L": " + getLastErrorMsg(dwError));
	}

	void deleteKey()
	{
		{
			DWORD dwError = ::RegOpenKey(m_root_key, m_path.c_str(), &m_local_key);
			if (dwError != ERROR_SUCCESS)
				raiseWError(L"Opening key failed: " + m_input_path + L": " + getLastErrorMsg(dwError));
		}

		{
			DWORD dwError = ::RegDeleteTree(m_local_key, m_path.c_str());
			if (dwError != ERROR_SUCCESS && dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_TOO_MANY_OPEN_FILES)
				raiseWError(L"Deleting key failed: " + m_input_path + L": " + getLastErrorMsg(dwError));
		}
	}

	object::list getSubKeys()
	{
		DWORD dwError = ::RegOpenKey(m_root_key, m_path.c_str(), &m_local_key);
		if (dwError != ERROR_SUCCESS)
			raiseWError(L"Opening key failed: " + m_input_path + L": " + getLastErrorMsg(dwError));

		const size_t MAX_VALUE_LEN = 16 * 1024;
		std::unique_ptr<wchar_t[]> value_name(new wchar_t[MAX_VALUE_LEN + 1]);
		value_name[MAX_VALUE_LEN] = '\0';

		object::list retVal;
		DWORD result = ERROR_SUCCESS;
		for (DWORD e = 0; ; ++e)
		{
			result = ::RegEnumKey(m_local_key, e, value_name.get(), MAX_VALUE_LEN);
			if (result == ERROR_NO_MORE_ITEMS)
				break;
			else if (result == ERROR_SUCCESS)
				retVal.push_back(std::wstring(value_name.get()));
			else
				raiseWError(L"Enumerating key failed: " + m_input_path + L": " + getLastErrorMsg(dwError));
		}
		return retVal;
	}

	void putKeySettings()
	{
		if (m_params.size() != 2 || m_params[1].type() != object::INDEX)
			raiseError("msreg_put_settings takes a registry key and an index of settings to put");
		object::index index = m_params[1].indexVal();

		DWORD dwError = ::RegOpenKey(m_root_key, m_path.c_str(), &m_local_key);
		if (dwError != ERROR_SUCCESS)
			raiseWError(L"Opening key failed: " + m_input_path + L": " + getLastErrorMsg(dwError));

		for (const auto& name : index.keys())
		{
			if (name.type() != object::STRING)
			{
				raiseWError(L"msreg_put_settings index key is not a string: " + name.toString());
			}

			object val = index.get(name);
			if (val.type() == object::NOTHING)
			{
				dwError = ::RegDeleteValue(m_local_key, name.stringVal().c_str());
				if (dwError != ERROR_SUCCESS)
					break;
			}
			else if (val.type() == object::NUMBER)
			{
				int64_t num_val = int64_t(round(val.numberVal()));
				if (num_val < 0)
					raiseWError(L"msreg_put_settings value must be a positive integer: " + name.toString());
				else if (num_val > MAXDWORD)
					raiseWError(L"msreg_put_settings value must not exceed DWORD capacity: " + name.toString());
				DWORD dw_val = DWORD(num_val);
				dwError = ::RegSetKeyValue(m_local_key, nullptr, name.stringVal().c_str(), REG_DWORD, LPCWSTR(&dw_val), sizeof(dw_val));
				if (dwError != ERROR_SUCCESS)
					raiseWError(L"Setting number value failed: " + m_input_path + L": " + name.stringVal() + L": " + getLastErrorMsg(dwError));
			}
			else if (val.type() == object::STRING)
			{
				dwError = ::RegSetKeyValue(m_local_key, nullptr, name.stringVal().c_str(), REG_SZ, val.stringVal().c_str(), DWORD((val.stringVal().length() + 1) * sizeof(wchar_t)));
				if (dwError != ERROR_SUCCESS)
					raiseWError(L"Setting string value failed: " + m_input_path + L": " + name.stringVal() + L": " + getLastErrorMsg(dwError));
			}
			else
				raiseWError(L"msreg_put_settings value is not null, number, or string: " + val.toString());
		}
	}

	object::index getKeySettings()
	{
		object::index ret_val;
		DWORD dwError = ::RegOpenKey(m_root_key, m_path.c_str(), &m_local_key);
		if (dwError != ERROR_SUCCESS)
			raiseWError(L"Opening key failed: " + m_input_path + L": " + getLastErrorMsg(dwError));

		const size_t MAX_VALUE_LEN = 16 * 1024;
		std::unique_ptr<wchar_t[]> value_name(new wchar_t[MAX_VALUE_LEN + 1]);
		value_name[MAX_VALUE_LEN] = '\0';
		for (DWORD i = 0; ; ++i)
		{
			DWORD val_len = MAX_VALUE_LEN;
			DWORD dwValRet =
				::RegEnumValue(m_local_key, i, value_name.get(), &val_len, nullptr, nullptr, nullptr, nullptr);
			if (dwValRet != ERROR_SUCCESS)
				break;

			const DWORD flags = RRF_RT_REG_DWORD | RRF_RT_REG_SZ;
			DWORD type = 0;
			DWORD data_len = 0;
			dwError =
				::RegGetValue(m_local_key, nullptr, value_name.get(), flags, &type, nullptr, &data_len);
			if (dwError != ERROR_SUCCESS && dwError != ERROR_MORE_DATA)
				break;
			if (type == REG_DWORD)
			{
				DWORD data_val = 0;
				dwError =
					::RegGetValue(m_local_key, nullptr, value_name.get(), flags, &type, &data_val, &data_len);
				if (dwError != ERROR_SUCCESS)
					raiseWError(L"Getting DWORD value failed: " + m_input_path + L": " + getLastErrorMsg(dwError));
				ret_val.set(std::wstring(value_name.get()), double(data_val));
			}
			else if (type == REG_SZ)
			{
				std::unique_ptr<wchar_t[]> value(new wchar_t[data_len + 1]);
				dwError =
					::RegGetValue(m_local_key, nullptr, value_name.get(), flags, &type, value.get(), &data_len);
				if (dwError != ERROR_SUCCESS)
					raiseWError(L"Getting string value failed: " + m_input_path + L": " + getLastErrorMsg(dwError));
				ret_val.set(std::wstring(value_name.get()), std::wstring(value.get()));
			}
			//else - omitted
		}

		return ret_val;
	}

private:
	object::list m_params;

	std::wstring m_input_path;

	HKEY m_root_key;
	std::wstring m_path;

	HKEY m_local_key;
};
