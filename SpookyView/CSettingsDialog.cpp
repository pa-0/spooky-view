#include "stdafx.h"
#include "CSettingsDialog.h"
#include <windowsx.h>
#include <shlwapi.h>
#include <memory>
#include "ISettingsManager.h"
#include "SpookyView.h"

CSettingsDialog::CSettingsDialog(HINSTANCE hInstance, HWND hParent) : CModelessDialog(hInstance, hParent)
{
}

BOOL CSettingsDialog::SetupDialog()
{
	key = IDD_SETTINGS;
	this->dialogResource = MAKEINTRESOURCE(IDD_SETTINGS);
	return TRUE;
}

INT_PTR CALLBACK CSettingsDialog::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetFormValues(hDlg);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDOK:
				ApplySettings();
			case IDCANCEL:
				DestroyWindow(hDlg);
				return TRUE;
			break;
		}
		break;
	}
	return FALSE;
}

void CSettingsDialog::ApplySettings()
{
	HWND autoStartupCheckbox = GetDlgItem(hWnd, IDC_CHECKBOX_AUTO_STARTUP);
	if (Button_GetCheck(autoStartupCheckbox))
	{
		AddAutoRun();
	}
	else
	{
		RemoveAutoRun();
	}
	HWND disableUpdateCheck = GetDlgItem(hWnd, IDC_CHECKBOX_DISABLE_UPDATE_CHECK);
	settingsManager->SetDisableUpdateCheck(Button_GetCheck(disableUpdateCheck));
	HWND skipIntroCheck = GetDlgItem(hWnd, IDC_CHECKBOX_SKIP_WELCOME);
	settingsManager->SetSkipWelcome(Button_GetCheck(skipIntroCheck));
	if (cIntroDialog && cIntroDialog->hasInitInstance())
	{
		cIntroDialog->setSkipWelcomeCheckbox(Button_GetCheck(skipIntroCheck));
	}
}

void CSettingsDialog::SetFormValues(HWND hDlg)
{
	HWND skipIntroCheckbox = GetDlgItem(hDlg, IDC_CHECKBOX_SKIP_WELCOME);
	int skipValue = settingsManager->GetSkipWelcome();
	if (skipValue == 1)
	{
		Button_SetCheck(skipIntroCheckbox, TRUE);
	}

	HWND autoStartupCheckbox = GetDlgItem(hDlg, IDC_CHECKBOX_AUTO_STARTUP);
#ifdef PACKAGING_STORE
	ShowWindow(autoStartupCheckbox, SW_HIDE);
#else
	if (HasAutoRunValue())
	{
		Button_SetCheck(autoStartupCheckbox, TRUE);
	}
#endif // PACKAGING_STORE
	HWND disableUpdateCheckbox = GetDlgItem(hDlg, IDC_CHECKBOX_DISABLE_UPDATE_CHECK);
#ifdef UNICODE
	if (settingsManager->GetDisableUpdateCheck())
	{
		Button_SetCheck(disableUpdateCheckbox, TRUE);
	}
#else
	ShowWindow(disableUpdateCheckbox, SW_HIDE);
#endif
}

BOOL CSettingsDialog::HasAutoRunValue()
{
	DWORD keyType;
	TCHAR keyData[MAX_PATH + 1];
	DWORD keyDataSize = sizeof(keyData);
	auto result = SHGetValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), _T("Spooky View"), &keyType, keyData, &keyDataSize);
	if (result == ERROR_SUCCESS && keyType == REG_SZ)
	{
		return TRUE;
	}
	return FALSE;
}

void CSettingsDialog::AddAutoRun()
{
	HKEY hKey;
	TCHAR programPath[MAX_PATH];
	GetModuleFileName(0, programPath, sizeof(programPath));
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		if (RegSetValueEx(hKey, _T("Spooky View"), 0, REG_SZ, (LPBYTE)programPath, sizeof(programPath)) != ERROR_SUCCESS)
		{
			//Show error message
		}
		RegCloseKey(hKey);
	}
}

void CSettingsDialog::RemoveAutoRun()
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		if (RegDeleteValue(hKey, _T("Spooky View")) != ERROR_SUCCESS)
		{
			//Show error message
		}
		RegCloseKey(hKey);
	}
}