module;

#include <Windows.h>
#include <string>
#include <format>

export module PersistentKeyValueStorage;

export namespace PersistentKeyValueStorage
{
	std::string appPath = "MercuryEditor";

	export void SetValue(const std::string& key, const std::string& value)
	{
		// Create the full path for the registry key
		std::string fullPath = "Software\\" + appPath;
		
		HKEY hKey;
		DWORD disposition;
		
		// Create or open the registry key
		LONG result = RegCreateKeyExA(
			HKEY_CURRENT_USER,
			fullPath.c_str(),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_WRITE,
			NULL,
			&hKey,
			&disposition
		);
		
		if (result == ERROR_SUCCESS)
		{
			// Write the value to the registry
			RegSetValueExA(
				hKey,
				key.c_str(),
				0,
				REG_SZ,
				reinterpret_cast<const BYTE*>(value.c_str()),
				static_cast<DWORD>(value.length() + 1) // Include null terminator
			);
			
			// Close the registry key
			RegCloseKey(hKey);
		}
	}
	
	export std::string GetStringValue(const std::string& key, const std::string& defaultValue)
	{
		// Create the full path for the registry key
		std::string fullPath = "Software\\" + appPath;
		
		HKEY hKey;
		
		// Open the registry key
		LONG result = RegOpenKeyExA(
			HKEY_CURRENT_USER,
			fullPath.c_str(),
			0,
			KEY_READ,
			&hKey
		);
		
		if (result != ERROR_SUCCESS)
		{
			return defaultValue;
		}
		
		// Query the value size first
		DWORD valueType;
		DWORD dataSize = 0;
		result = RegQueryValueExA(
			hKey,
			key.c_str(),
			NULL,
			&valueType,
			NULL,
			&dataSize
		);
		
		if (result != ERROR_SUCCESS || valueType != REG_SZ)
		{
			RegCloseKey(hKey);
			return defaultValue;
		}
		
		// Allocate buffer and get the value
		std::string value(dataSize, '\0');
		result = RegQueryValueExA(
			hKey,
			key.c_str(),
			NULL,
			NULL,
			reinterpret_cast<BYTE*>(&value[0]),
			&dataSize
		);
		
		// Close the registry key
		RegCloseKey(hKey);
		
		if (result != ERROR_SUCCESS)
		{
			return defaultValue;
		}
		
		// Remove extra null terminator if present
		if (!value.empty() && value.back() == '\0')
		{
			value.pop_back();
		}
		
		return value;
	}
}