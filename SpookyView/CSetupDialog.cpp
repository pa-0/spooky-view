#include "stdafx.h"
#include "CSetupDialog.h"
#include "CAddAppDialog.h"
#include "CAddWindowDialog.h"
#include <commctrl.h>
#include "CSettings.h"
#include "CProgramSetting.h"
#include "ISettingsManager.h"
#include "String.h"
#include <vector>
#include <memory>
#include <windowsx.h>
#include "Defines.h"

extern std::unique_ptr<ISettingsManager> settingsManager;
extern void SetWindowsTransparency();
extern void ResetWindowsTransparency();

#define TRANSPARENCY_TRACKER_STEPS (100.0 / 255.0)

CSetupDialog::CSetupDialog(HINSTANCE hInstance) : CModelessDialog(hInstance)
{
	this->CopySettings();
}

CSetupDialog::~CSetupDialog()
{
}

BOOL CSetupDialog::SetupDialog()
{
	this->dialogResource = MAKEINTRESOURCE(IDD_SETUP);
	return TRUE;
}

INT_PTR CALLBACK CSetupDialog::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NOTIFY:
	{
		LPNMHDR notifyMessage = (LPNMHDR)lParam;
		if (notifyMessage->code == LVN_ITEMCHANGED)
		{
			switch (notifyMessage->idFrom)
			{
			case IDC_LIST_APPS:
				ProgramsListNotified();
				return TRUE;

			case IDC_LIST_WINDOWS:
				WindowsListNotified();
				return TRUE;
			}
		}
	}
	break;

	case WM_HSCROLL:
	{
		auto senderHwnd = (HWND)lParam;
		LONG identifier = GetWindowLong(senderHwnd, GWL_ID);
		if (identifier == IDC_SLIDER_FOREGROUND || identifier == IDC_SLIDER_BACKGROUND)
		{
			if (LOWORD(wParam) == TB_THUMBPOSITION || LOWORD(wParam) == TB_THUMBTRACK)
			{
				BYTE value = HIWORD(wParam) & 0xff;
				SetAlpha(value, senderHwnd);
			}
			else
			{
				BYTE value = SendMessage(senderHwnd, TBM_GETPOS, 0, 0) & 0xff;
				SetAlpha(value, senderHwnd);
			}
		}
		return TRUE;
	}

	case WM_INITDIALOG:
		//Create the listviews
		appsListView = std::make_unique<ListView>(hDlg, IDC_LIST_APPS);
		windowsListView = std::make_unique<ListView>(hDlg, IDC_LIST_WINDOWS);
		enabledCheckbox = std::make_unique<Checkbox>(hDlg, IDC_CHECKBOX_ENABLE_TRANSPARENCY);

		PopulateProcessList();
		SetTrackbarRanges(hDlg);
		//Set the trackbars on the global settings
		this->currentAlphaSettings = &newSettings->alphaSettings;
		SetTrackbars();
		SetCheckboxes();
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_CHECKBOX_ENABLE_TRANSPARENCY:
				EnabledCheckboxNotified();
				return TRUE;
			case IDAPPLY:
				ApplySettings();
				return TRUE;
			case IDOK:
				ApplySettings();
			case IDCANCEL:
				DestroyWindow(hDlg);
				return TRUE;
			case IDC_BUTTON_APP_REMOVE:
				newSettings->programs->erase(this->currentProgramName);
				this->appsListView->DeleteSelectedItem();
				return TRUE;
			case IDC_BUTTON_WINDOW_REMOVE:
				this->currentProgram->windows->erase(this->currentWindowClassName);
				this->windowsListView->DeleteSelectedItem();
				return TRUE;
			case IDC_BUTTON_APP_ADD:
				{
					auto appDialog = std::make_unique<CAddAppDialog>(this->hInstance, this->hWnd);
					appDialog->InitInstance();
					if (appDialog->GetResult() == 1)
					{
						LPTSTR programName = appDialog->GetSelectedProcess();

						auto progSettings = new CProgramSetting();
						//Make process name lower case
						newSettings->ToLowerCase(programName);
						newSettings->programs->insert(std::pair<t_string, CProgramSetting*>(programName, progSettings));
						this->appsListView->AddItem(programName);
					}
				}
				return TRUE;
			case IDC_BUTTON_WINDOW_ADD:
				{
					auto windowDialog = std::make_unique<CAddWindowDialog>(this->hInstance, this->hWnd);
					windowDialog->InitInstance(this->currentProgramName);
					if (windowDialog->GetResult() == 1)
					{
						auto windowClassName = windowDialog->GetSelectedWindowClass();

						auto windowSettings = new CWindowSetting();
						this->currentProgram->windows->insert(std::pair<t_string, CWindowSetting*>(windowClassName, windowSettings));
						this->windowsListView->AddItem(windowClassName);
					}
				}
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

void CSetupDialog::ApplySettings()
{
	ResetWindowsTransparency();
	settingsManager->ApplyNewSettings(newSettings.get());
	settingsManager->SaveSettings();
	SetWindowsTransparency();
}

void CSetupDialog::CopySettings()
{
	newSettings = std::make_unique<CSettings>(*settingsManager->GetSettings());
}

void CSetupDialog::ProgramsListNotified()
{
	int index = this->appsListView->GetSelectedIndex();
	if (index > 0)
	{
		TCHAR textBuffer[MAX_PATH];
		LPTSTR text = this->appsListView->GetTextByIndex(index, textBuffer);

		auto program = newSettings->programs->find(text);
		if (program != newSettings->programs->end())
		{
			this->currentProgramName = program->first;
			this->currentProgram = program->second;
			this->currentAlphaSettings = &program->second->alphaSettings;
			SetTrackbars();
			SetCheckboxes();
			PopulateWindowsList(program->second);
		}
		this->SetFormVisibility(TRUE);
	}
	else if (index == 0)
	{
		//When first item (all programs) is selected, get the global settings
		this->currentProgram = 0;
		this->currentAlphaSettings = &newSettings->alphaSettings;
		SetTrackbars();
		SetCheckboxes();
		PopulateWindowsList();
		this->SetFormVisibility(TRUE);
	} 
	else
	{
		//When no item is selected, hide the controls and clear windows list
		this->SetFormVisibility(FALSE);
		this->windowsListView->DeleteAllItems();
		SetButtonEnableState(IDC_BUTTON_WINDOW_ADD, false);
		SetButtonEnableState(IDC_BUTTON_WINDOW_REMOVE, false);
	}
	if (index >= 0) {
		//Select the first item available
		this->windowsListView->SetSelectedItem(0);
	}
	//Enable the remove button when a program is selected
	SetButtonEnableState(IDC_BUTTON_APP_REMOVE, index > 0);
}

void CSetupDialog::WindowsListNotified()
{
	int index = this->windowsListView->GetSelectedIndex();
	if (index > 0)
	{
		TCHAR textBuffer[MAX_WINDOW_CLASS_NAME];
		LPTSTR text = this->windowsListView->GetTextByIndex(index, textBuffer);

		auto window = currentProgram->windows->find(text);
		if (window != currentProgram->windows->end())
		{
			this->currentWindowClassName = window->first;
			this->currentAlphaSettings = &window->second->alphaSettings;
			SetTrackbars();
			SetCheckboxes();
		}
		this->SetFormVisibility(TRUE);
	}
	else if (index == 0)
	{
		//When first item (all windows) is selected, get the program global settings
		//If no program is selected, get the global settings
		if (this->currentProgram) {
			this->currentAlphaSettings = &currentProgram->alphaSettings;
		}
		else {
			this->currentAlphaSettings = &newSettings->alphaSettings;
		}
		SetTrackbars();
		SetCheckboxes();
		this->SetFormVisibility(TRUE);
	}
	else
	{
		//When no item is selected, hide the form
		this->SetFormVisibility(FALSE);
	}
	//Enable the remove button when a window is selected
	SetButtonEnableState(IDC_BUTTON_WINDOW_REMOVE, index > 0);
	//When a program is selected, enable the add button
	if (this->currentProgram) {
		SetButtonEnableState(IDC_BUTTON_WINDOW_ADD, true);
	}
	else
	{
		SetButtonEnableState(IDC_BUTTON_WINDOW_ADD, false);
	}
}

void CSetupDialog::EnabledCheckboxNotified()
{
	currentAlphaSettings->enabled = enabledCheckbox->GetCheckState();
}

void CSetupDialog::PopulateProcessList()
{
	auto programs = newSettings->programs.get();
	this->appsListView->AddItem(_T("[All other programs]"));
	for (auto const program : *programs)
	{
		this->appsListView->AddItem(program.first);
	}
}

void CSetupDialog::PopulateWindowsList(CProgramSetting* settings)
{
	this->windowsListView->DeleteAllItems();
	this->windowsListView->AddItem(_T("[All other windows]"));
	if (settings) {
		for (auto const& window : *settings->windows)
		{
			this->windowsListView->AddItem(window.first);
		}
	}
}

void CSetupDialog::SetTrackbarRanges(HWND hWnd)
{
	HWND foregroundTrackbar = GetDlgItem(hWnd, IDC_SLIDER_FOREGROUND);
	HWND backgroundTrackbar = GetDlgItem(hWnd, IDC_SLIDER_BACKGROUND);
	SendMessage(foregroundTrackbar, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)100);
	SendMessage(backgroundTrackbar, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)100);
	SendMessage(foregroundTrackbar, TBM_GETRANGEMIN, (WPARAM)TRUE, (LPARAM)10);
	SendMessage(backgroundTrackbar, TBM_GETRANGEMIN, (WPARAM)TRUE, (LPARAM)10);
}

void CSetupDialog::SetTrackbars()
{
	HWND foregroundTrackbar = GetDlgItem(this->hWnd, IDC_SLIDER_FOREGROUND);
	HWND backgroundTrackbar = GetDlgItem(this->hWnd, IDC_SLIDER_BACKGROUND);

	auto test = (LPARAM)ceil(currentAlphaSettings->foreground * TRANSPARENCY_TRACKER_STEPS);

	SendMessage(foregroundTrackbar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)ceil(currentAlphaSettings->foreground * TRANSPARENCY_TRACKER_STEPS));
	SendMessage(backgroundTrackbar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)ceil(currentAlphaSettings->background * TRANSPARENCY_TRACKER_STEPS));
}

void CSetupDialog::SetCheckboxes()
{
	enabledCheckbox->SetCheckState(currentAlphaSettings->enabled);
}

void CSetupDialog::SetAlpha(BYTE value, HWND trackbar)
{
	LONG identifier = GetWindowLong(trackbar, GWL_ID);
	switch (identifier)
	{
	case IDC_SLIDER_FOREGROUND:
		currentAlphaSettings->foreground = (BYTE)ceil(value / TRANSPARENCY_TRACKER_STEPS);
		break;
	case IDC_SLIDER_BACKGROUND:
		currentAlphaSettings->background = (BYTE)ceil(value / TRANSPARENCY_TRACKER_STEPS);
		break;
	}
}

void CSetupDialog::SetFormVisibility(bool show)
{
	const int itemIds[] = {IDC_CHECKBOX_ENABLE_TRANSPARENCY, IDC_STATIC_TRANSPARENCY, IDC_STATIC_FOREGROUND, IDC_SLIDER_FOREGROUND, IDC_STATIC_BACKGROUND, IDC_SLIDER_BACKGROUND };
	for (const auto& itemId : itemIds) {
		auto item = GetDlgItem(this->hWnd, itemId);
		ShowWindow(item, show ? SW_SHOW : SW_HIDE);
	}
}

void CSetupDialog::SetButtonEnableState(int controlId, bool show)
{
	auto item = GetDlgItem(this->hWnd, controlId);
	EnableWindow(item, show);
}