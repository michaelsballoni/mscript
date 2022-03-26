#include "pch.h"

using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"msreg_create_key",
		L"msreg_delete_key",

		L"msreg_get_settings",
		L"msreg_put_settings",
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
		
		object::list params = module_utils::getParams(parametersJson);
		if (params.size() < 1)
			raiseError("Registry functions take a registry key string parameter");
		object registry_key_param = params[0];
		if (registry_key_param.type() != object::STRING)
			raiseError("Registry functions take a registry key string parameter");
		std::wstring full_registry_key = registry_key_param.stringVal();
		
		// FORNOW
		HKEY root = nullptr;
		std::wstring registry_key;
		{
			size_t first_slash = full_registry_key.find('\\');
			if (first_slash == std::wstring::npos)
				raiseWError(L"Invalid registry key (no slash): " + full_registry_key);

			std::wstring reg_root = toUpper(full_registry_key.substr(first_slash));
			if (reg_root.empty())
				raiseWError(L"Invalid registry key (invalid root): " + full_registry_key);

			registry_key = full_registry_key.substr(first_slash + 1);
			if (registry_key.empty())
				raiseWError(L"Invalid registry key (empty key): " + full_registry_key);

			if (reg_root == L"HKCR" || reg_root == L"HKEY_CLASSES_ROOT")
				root = HKEY_CLASSES_ROOT;
			else if (reg_root == L"HKCC" || reg_root == L"HKEY_CURRENT_CONFIG")
				root = HKEY_CURRENT_CONFIG;
			else if (reg_root == L"HKCU" || reg_root == L"HKEY_CURRENT_USER")
				root = HKEY_CURRENT_USER;
			else if (reg_root == L"HKLM" || reg_root == L"HKEY_LOCAL_MACHINE")
				root = HKEY_LOCAL_MACHINE;
			else if (reg_root == L"HCU" || reg_root == L"HKEY_USERS")
				root = HKEY_USERS;
			else
				raiseWError(L"Invalid registry key: " + full_registry_key);
		}

		// FORNOW
		object retVal = true;
		DWORD dwError = ERROR_SUCCESS;
		if (funcName == L"msreg_create_key")
		{
			HKEY newKey = nullptr;
			dwError = ::RegCreateKey(root, registry_key.c_str(), &newKey);
			::RegCloseKey(newKey);
		}
		else if (funcName == L"msreg_delete_key")
		{
			dwError = ::RegDeleteKey(root, registry_key.c_str());
		}
		/*
				L"msreg_get_settings",
		L"msreg_put_settings",

		*/
		else if (funcName == L"msreg_get_settings")
		{
			retVal = object::index();
			HKEY key = nullptr;
			dwError = ::RegOpenKey(root, registry_key.c_str(), &key);
			if (dwError == ERROR_SUCCESS)
			{
				const size_t MAX_VALUE_LEN = 16 * 1024;
				wchar_t value_name[MAX_VALUE_LEN + 1];
				for (DWORD i = 0; ; ++i)
				{
					DWORD val_len = MAX_VALUE_LEN;
					DWORD dwValRet =
						::RegEnumValue(key, i, value_name, &val_len, nullptr, nullptr, nullptr, nullptr);
					if (dwValRet != ERROR_SUCCESS)
						break;

					const DWORD flags = RRF_RT_REG_DWORD | RRF_RT_REG_SZ;
					DWORD type = 0;
					DWORD data_len = 0;
					dwError = 
						::RegGetValue(root, registry_key.c_str(), value_name, flags, &type, nullptr, &data_len);
					if (dwError != ERROR_SUCCESS && dwError != ERROR_MORE_DATA)
						break;
					if (type == REG_DWORD)
					{
						DWORD data_val = 0;
						dwError =
							::RegGetValue(root, registry_key.c_str(), value_name, flags, &type, &data_val, &data_len);
						if (dwError != ERROR_SUCCESS)
							break;
						retVal.indexVal().set(std::wstring(value_name), double(data_val));
					}
					else if (type == REG_SZ)
					{
						std::unique_ptr<wchar_t[]> value(new wchar_t[data_len + 1]);
						dwError =
							::RegGetValue(root, registry_key.c_str(), value_name, flags, &type, value.get(), &data_len);
						if (dwError != ERROR_SUCCESS)
							break;
						retVal.indexVal().set(std::wstring(value_name), std::wstring(value.get()));
					}
					else
					{
						raiseWError(L"Unhandled registry value type: " + full_registry_key + L": " + std::to_wstring(type));
					}
				}
			}
			::RegCloseKey(key);
		}
		else if (funcName == L"msreg_put_settings")
		{
			if (params.size() != 2)
				raiseError("msreg_put_settings takes a registry key and an index of settings to make");
			object index_param = params[0];
			if (index_param.type() != object::INDEX)
				raiseError("msreg_put_settings takes a registry key and an index of settings to make");
			object::index index = index_param.indexVal();
			
			HKEY key = nullptr;
			dwError = ::RegOpenKey(root, registry_key.c_str(), &key);
			if (dwError == ERROR_SUCCESS)
			{
				for (const auto& name : index.keys())
				{
					if (name.type() != object::STRING)
					{
						raiseWError(L"msreg_put_settings key is not string: " + name.toString());
					}

					object val = index.get(name);
					if (val.type() == object::NOTHING)
					{
						dwError = ::RegDeleteValue(key, name.stringVal().c_str());
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
						dwError = ::RegSetValue(key, name.stringVal().c_str(), REG_DWORD, LPCWSTR(&dw_val), sizeof(dw_val));
						if (dwError != ERROR_SUCCESS)
							break;
					}
					else if (val.type() == object::STRING)
					{
						dwError = ::RegSetValue(key, name.stringVal().c_str(), REG_SZ, val.stringVal().c_str(), val.stringVal().length());
						if (dwError != ERROR_SUCCESS)
							break;
					}
					else
						raiseWError(L"msreg_put_settings value is not null, number, or string: " + val.toString());

				}
			}
			::RegCloseKey(key);
		}
		else
			raiseWError(L"Unknown mscript-registry function: " + funcName);

		return module_utils::jsonStr(double(dwError));
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
