// SpookyView.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "SpookyView.h"
#include "AppMain.h"
#include "CMainWindow.h"
#include "ISettingsManager.h"
#include "UpdateResponse.h"
#include "WindowsEnum.h"

// Global Variables:
HINSTANCE hInst; // current instance
HWND mainHwnd; // Main window hwnd
HICON spookyIcon;
PGNSI isImmersive;
std::unique_ptr<CMainWindow> mainWindow;
std::unique_ptr<CSettingsDialog> cSettingsDialog;
std::unique_ptr<CSetupDialog> cSetupDialog;
std::unique_ptr<CIntroDialog> cIntroDialog;
std::unique_ptr<ISettingsManager> settingsManager;
UpdateResponse updateResponse;
WindowsEnum windowsEnum;
bool isWindows8;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance;
	AppMain appMain;
	return appMain.Run();
}
