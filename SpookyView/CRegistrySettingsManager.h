#include "ISettingsManager.h"
#include "Unicode.h"
#pragma once

class CRegistrySettingsManager : public ISettingsManager
{
public:
	CRegistrySettingsManager();
	~CRegistrySettingsManager();
	void LoadSettings();
	bool SaveSettings();
	CSettings* GetSettings();
	void ApplyNewSettings(CSettings *newSettings);
	void AddSkipVersionKey(tstring versionNumber);
	BOOL ShouldSkipVersion(tstring versionNumber);
	BOOL GetDisableUpdateCheck();
	void SetDisableUpdateCheck(BOOL state);
	BOOL GetSkipWelcome();
	void SetSkipWelcome();
protected:
	std::unique_ptr<CSettings> settings;
	void SaveAlphaSettingsValues(HKEY key, CAlphaSettings values);
	HKEY registryRootKey;
	BOOL ReadKeyByteValue(HKEY key, TCHAR* valueName, BYTE& value);
	void ReadAlphaValues(HKEY key, CAlphaSettings* settings);
	LSTATUS ClearProgramSettings();
	BOOL ReadValue(TCHAR* subkey, TCHAR* valueName, DWORD expectedKeyType, void *dataBuffer, DWORD dataBufferSize);
	BOOL SaveValue(HKEY hKey, TCHAR* valueName, DWORD keyType, BYTE* value, DWORD valueSize = 0);
	BOOL SaveValue(TCHAR* subkey, TCHAR* valueName, DWORD keyType, BYTE* value);
};