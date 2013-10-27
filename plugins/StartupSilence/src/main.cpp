/*
Copyright 2012-2013 Vladimir Lyubimov
Copyright 2012-2013 Miranda-NG team,

all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//HELPME: need new icons for TTB bottons

#include "StartupSilence.h"

HINSTANCE hInst;
int hLangpack;
HANDLE hSSMenuToggleOnOff;
HANDLE GetIconHandle(char *szIcon);
HANDLE hOptionsInitialize;
HANDLE hTTBarloaded = NULL;
HANDLE Buttons = NULL;
INT_PTR CALLBACK DlgProcOptions(HWND, UINT, WPARAM, LPARAM);
int InitializeOptions(WPARAM wParam,LPARAM lParam);
int DisablePopup(WPARAM wParam, LPARAM lParam);
int ModulesLoaded(WPARAM wParam, LPARAM lParam);
static int CreateTTButtons(WPARAM wParam, LPARAM lParam);
void RemoveTTButtons();
void EnablePopupModule();
BYTE Enabled;
DWORD delay;
BYTE PopUp;
DWORD PopUpTime;
BYTE MenuItem;
BYTE TTBButtons;
BYTE timer;
char hostname[MAX_PATH] = "";
char EnabledComp[MAX_PATH] = "";
char DelayComp[MAX_PATH] = "";
char PopUpComp[MAX_PATH] = "";
char PopUpTimeComp[MAX_PATH] = "";
char MenuitemComp[MAX_PATH] = "";
char TTBButtonsComp[MAX_PATH] = "";

static LIST<void> ttbButtons(1);

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESC,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {7B856B6A-D48F-4f54-B8D6-C8D86D02FFC2}
	0x7b856b6a, 0xd48f, 0x4f54, {0xb8, 0xd6, 0xc8, 0xd8, 0x6d, 0x2, 0xff, 0xc2}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

INT_PTR StartupSilence()
{
	InitSettings();
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	mir_forkthread((pThreadFunc)AdvSt, NULL);
	CreateServiceFunction(SS_SERVICE_NAME, SturtupSilenceEnabled);
	IsMenu();
	HookEvent(ME_OPT_INITIALISE, InitializeOptions);
	return 0;
}

extern "C" __declspec(dllexport) int Load(void)
{
	mir_getLP(&pluginInfo);
	StartupSilence();
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	if (hTTBarloaded != NULL){
		UnhookEvent(hTTBarloaded);
	}
	return 0;
}

int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	HookEvent(ME_POPUP_FILTER, DisablePopup);
	hTTBarloaded = HookEvent(ME_TTB_MODULELOADED, CreateTTButtons);
	if (TTBButtons == 1 && hTTBarloaded != NULL) {
		Icon_Register(hInst, "Toolbar/"MENU_NAME, iconttbList, SIZEOF(iconttbList), MENU_NAME);
		RemoveTTButtons();
		CreateTTButtons(0,0);
	}
	return 0;
}

int DisablePopup(WPARAM wParam, LPARAM lParam)
{
	if (timer == 2)
		return 1;
	return 0;
}

void EnablePopupModule()
{
	if (ServiceExists(POPUPONOFFPP) && db_get_b(NULL,"Popup", "ModuleIsEnabled", 0) == 0) {
		CallService(POPUPONOFFPP, NULL, NULL);
	}
	if (ServiceExists(POPUPONOFF) && db_get_b(NULL, "YAPP", "Enabled", 0) == 0) {
		CallService(POPUPONOFF, NULL, NULL);
	}
}

void InitSettings()
{
	if(gethostname(hostname, SIZEOF(hostname)) == 0){
		mir_snprintf(EnabledComp, SIZEOF(EnabledComp), "%s_Enabled", hostname);
		mir_snprintf(DelayComp, SIZEOF(DelayComp), "%s_Delay", hostname);
		mir_snprintf(PopUpComp, SIZEOF(PopUpComp), "%s_PopUp", hostname);
		mir_snprintf(PopUpTimeComp, SIZEOF(PopUpTimeComp), "%s_PopUpTime", hostname);
		mir_snprintf(MenuitemComp, SIZEOF(MenuitemComp), "%s_MenuItem", hostname);
		mir_snprintf(TTBButtonsComp, SIZEOF(TTBButtonsComp), "%s_TTBButtons", hostname);
	}
	//first run on the host, initial setting
	if (!(delay = db_get_dw(NULL, MODULE_NAME, DelayComp, 0)))
		DefSettings();
	//or load host settings
	else LoadSettings();
}
void DefSettings()
{
	db_set_dw(NULL, MODULE_NAME, DelayComp, 20);
	db_set_b(NULL, MODULE_NAME, EnabledComp, 1);
	db_set_b(NULL, MODULE_NAME, PopUpComp, 1);
	db_set_dw(NULL, MODULE_NAME, PopUpTimeComp, 5);
	db_set_b(NULL, MODULE_NAME, MenuitemComp, 1);
	db_set_b(NULL, MODULE_NAME, TTBButtonsComp, 0);
	LoadSettings();
}
void LoadSettings()
{
	Enabled = db_get_b(NULL, MODULE_NAME, EnabledComp, 0);
	delay = db_get_dw(NULL, MODULE_NAME, DelayComp, 0);
	PopUp = db_get_b(NULL, MODULE_NAME, PopUpComp, 0);
	PopUpTime = db_get_dw(NULL, MODULE_NAME, PopUpTimeComp, 0);
	MenuItem = db_get_b(NULL, MODULE_NAME, MenuitemComp, 0);
	TTBButtons = db_get_b(NULL, MODULE_NAME, TTBButtonsComp, 0);
	if (PopUpTime < 1)
		PopUpTime = (DWORD)1;
	if (PopUpTime > 30)
		PopUpTime = (DWORD)30;
	if (delay < 10)
		delay = (DWORD)10;
	if (delay > 300)
		delay = (DWORD)300;
	db_set_dw(NULL, MODULE_NAME, DelayComp, delay);
	db_set_dw(NULL, MODULE_NAME, PopUpTimeComp, PopUpTime);
}

void IsMenu()
{
	if (MenuItem == 1) {
		Icon_Register(hInst, MENU_NAME, iconList, SIZEOF(iconList), MENU_NAME);
		InitMenu();
	}
}

static INT_PTR AdvSt()
{
		if ((Enabled == 1)){
			POPUPDATAT ppd = {0};
			TCHAR * lptzText =L"";
			db_set_b(NULL, "Skin", "UseSound", 0);
			EnablePopupModule();

			if (PopUp == 1) {
				lptzText = ALL_DISABLED;
				ppd.lchIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)GetIconHandle(MENU_NAME));
				ppd.lchContact = NULL;
				ppd.iSeconds = PopUpTime;
				wcsncpy_s(ppd.lptzText, lptzText, size_t(lptzText));
				lptzText = TranslateT(MENU_NAME);
				wcsncpy_s(ppd.lptzContactName, lptzText, size_t(lptzText));
				PUAddPopupT(&ppd);
			}

			timer = 2;
			Sleep(delay * 1000);
			timer = 0;

			db_set_b(NULL, "Skin", "UseSound", 1);
			if (PopUp == 1) {
				lptzText = ALL_ENABLED;
				wcsncpy_s(ppd.lptzText, lptzText, size_t(lptzText));
				PUAddPopupT(&ppd);
			}
		}
	return 0;
}

static INT_PTR SturtupSilenceEnabled(WPARAM wParam, LPARAM lParam)
{
	db_set_b(NULL, MODULE_NAME, EnabledComp, !Enabled);
	LoadSettings();
	if (MenuItem == 1)
		UpdateMenu();
	if (PopUp == 1) {
		TCHAR * lptzText = Enabled == 1 ? S_MODE_CHANGEDON : S_MODE_CHANGEDOFF;
		POPUPDATAT ppd = {0};
		ppd.lchIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)(Enabled == 1 ? GetIconHandle(ENABLE_SILENCE) : GetIconHandle(DISABLE_SILENCE)));
		ppd.lchContact = NULL;
		ppd.iSeconds = PopUpTime;
		wcsncpy_s(ppd.lptzText, lptzText, size_t(lptzText));
		lptzText = TranslateT(MENU_NAME);
		wcsncpy_s(ppd.lptzContactName, lptzText, size_t(lptzText));
		PUAddPopupT(&ppd);
	}
	return 0;
}

static INT_PTR InitMenu()
{
	CLISTMENUITEM mi = {sizeof(mi)};
	mi.flags = CMIM_ALL;
	mi.position = 100000000;
	mi.icolibItem = GetIconHandle(MENU_NAME);
	mi.pszPopupName = MENU_NAME;
	mi.pszService = SS_SERVICE_NAME;
	hSSMenuToggleOnOff = Menu_AddMainMenuItem(&mi);
	UpdateMenu();
	return 0;
}

void UpdateMenu()
{
	CLISTMENUITEM mi = {sizeof(mi)};
	mi.pszName = (Enabled == 1 ? DISABLE_SILENCE : ENABLE_SILENCE);
	mi.flags = CMIM_NAME | CMIM_ALL;
	mi.icolibItem = (Enabled == 1 ? GetIconHandle(DISABLE_SILENCE) : GetIconHandle(ENABLE_SILENCE));
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hSSMenuToggleOnOff, (LPARAM)&mi);
	UpdateTTB();
}

void UpdateTTB()
{
	if (hTTBarloaded != NULL && TTBButtons == 1)
		CallService(MS_TTB_SETBUTTONSTATE, (WPARAM)Buttons, (Enabled == 1 ?  TTBST_RELEASED : TTBST_PUSHED));
}

static int CreateTTButtons(WPARAM wParam, LPARAM lParam)
{
	TTBButton ttb = {sizeof(ttb)};
	ttb.dwFlags = (Enabled == 1 ? 0 : TTBBF_PUSHED) | TTBBF_VISIBLE | TTBBF_ASPUSHBUTTON;
	ttb.pszService = SS_SERVICE_NAME;
	ttb.hIconHandleDn = GetIconHandle(DISABLE_SILENCETTB);
	ttb.hIconHandleUp = GetIconHandle(ENABLE_SILENCETTB);
	ttb.name = TTBNAME;
	ttb.pszTooltipUp = SS_IS_ON;
	ttb.pszTooltipDn = SS_IS_OFF;
	Buttons = TopToolbar_AddButton(&ttb);
	if (Buttons)
		ttbButtons.insert(Buttons);
	return 0;
}

void RemoveTTButtons()
{
	for (int i=ttbButtons.getCount()-1; i >= 0; i--)
		CallService(MS_TTB_REMOVEBUTTON, (WPARAM)ttbButtons[i], 0);
	ttbButtons.destroy();
}

HANDLE GetIconHandle(char *szIcon)
{
	char szSettingName[64];
	mir_snprintf(szSettingName, sizeof(szSettingName), "%s_%s", MENU_NAME, szIcon);
	return Skin_GetIconHandle(szSettingName);
}

int InitializeOptions(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {sizeof(odp)};
	odp.position = 0;
	odp.hInstance = hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_SSOPT);
	odp.pszGroup = LPGEN("Events");//FIXME: move to...Group?
	odp.pszTitle = MENU_NAME;
	odp.groupPosition = 910000000;
	odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = DlgProcOptions;
	Options_AddPage(wParam, &odp);
	return 0;
}

static INT_PTR CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		TranslateDialogDefault(hwndDlg);
		static BOOL initDlg=FALSE;
		LoadSettings();
		SetDlgItemText(hwndDlg,  IDC_HST, mir_a2t(hostname));
		CheckDlgButton(hwndDlg, IDC_DELAY, (Enabled == 1) ? BST_CHECKED:BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg, IDC_SSSPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwndDlg, IDC_SSTIME), 0);
		SendDlgItemMessage(hwndDlg, IDC_SSSPIN, UDM_SETRANGE32, 10, 300);
		SendDlgItemMessage(hwndDlg, IDC_SSSPIN, UDM_SETPOS, 0, MAKELONG((delay), 0));
		SendDlgItemMessage(hwndDlg, IDC_SSTIME, EM_LIMITTEXT, (WPARAM)3, 0);

		CheckDlgButton(hwndDlg, IDC_DELAY2, (PopUp == 1) ? BST_CHECKED:BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg, IDC_SSSPIN2, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwndDlg, IDC_SSPOPUPTIME), 0);
		SendDlgItemMessage(hwndDlg, IDC_SSSPIN2, UDM_SETRANGE32, 1, 30);
		SendDlgItemMessage(hwndDlg, IDC_SSSPIN2, UDM_SETPOS, 0, MAKELONG((PopUpTime), 0));
		SendDlgItemMessage(hwndDlg, IDC_SSPOPUPTIME, EM_LIMITTEXT, (WPARAM)3, 0);

		CheckDlgButton(hwndDlg, IDC_MENU, (MenuItem == 1) ? BST_CHECKED:BST_UNCHECKED);

		CheckDlgButton(hwndDlg, IDC_TTB, (TTBButtons == 1) ? BST_CHECKED:BST_UNCHECKED);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SSTIME:
			DWORD min;
			if ((HWND)lParam != GetFocus() || HIWORD(wParam) != EN_CHANGE) return FALSE;
				min = GetDlgItemInt(hwndDlg, IDC_SSTIME, NULL, FALSE);
			if (min == 0 && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_SSTIME)))
				SendDlgItemMessage(hwndDlg, IDC_SSSPIN, UDM_SETPOS, 0, MAKELONG((short) 1, 0));
			delay = (DWORD)db_set_dw(NULL, MODULE_NAME, DelayComp, (DWORD)(SendDlgItemMessage(hwndDlg, IDC_SSSPIN, UDM_GETPOS, 0, 0)));
			break;

		case IDC_SSPOPUPTIME:
			if ((HWND)lParam != GetFocus() || HIWORD(wParam) != EN_CHANGE) return FALSE;
				min = GetDlgItemInt(hwndDlg, IDC_SSPOPUPTIME, NULL, FALSE);
			if (min == 0 && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_SSPOPUPTIME)))
				SendDlgItemMessage(hwndDlg, IDC_SSSPIN2, UDM_SETPOS, 0, MAKELONG((short) 1, 0));
			PopUpTime = (DWORD)db_set_dw(NULL, MODULE_NAME, PopUpTimeComp, (DWORD)(SendDlgItemMessage(hwndDlg, IDC_SSSPIN2, UDM_GETPOS, 0, 0)));
			break;

		case IDC_DELAY:
			CallService(SS_SERVICE_NAME, 0, 0);
			break;

		case IDC_DELAY2:
			if (!(ServiceExists(POPUPONOFF) || ServiceExists(POPUPONOFFPP))) {
				MessageBox(0, NEEDPOPUP, NOTICE, MB_OK);
				CheckDlgButton(hwndDlg, IDC_DELAY2, BST_UNCHECKED);
				PopUp = (BYTE)db_set_b(NULL, MODULE_NAME, PopUpComp, 0);
			}
			else PopUp = (BYTE)db_set_b(NULL, MODULE_NAME, PopUpComp, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_DELAY2) == BST_CHECKED ? 1 : 0));
			break;

		case IDC_MENU:
			MenuItem = (BYTE)db_set_b(NULL, MODULE_NAME, MenuitemComp, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_MENU) == BST_CHECKED ? 1 : 0));
			break;

		case IDC_TTB:
			if (!hTTBarloaded) {
				MessageBox(0, NEEDTTBMOD, NOTICE, MB_OK);
				CheckDlgButton(hwndDlg, IDC_TTB, BST_UNCHECKED);
				TTBButtons = (BYTE)db_set_b(NULL, MODULE_NAME, TTBButtonsComp, 0);
			}
			else TTBButtons = (BYTE)db_set_b(NULL, MODULE_NAME, TTBButtonsComp, (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_TTB) == BST_CHECKED ? 1 : 0));
			break;
		case IDC_RESETDEFAULT:
			DefSettings();
			CheckDlgButton(hwndDlg, IDC_DELAY, (Enabled == 1) ? BST_CHECKED:BST_UNCHECKED);
			SendDlgItemMessage(hwndDlg, IDC_SSSPIN, UDM_SETPOS, 0, MAKELONG((delay), 0));
			CheckDlgButton(hwndDlg, IDC_DELAY2, (PopUp == 1) ? BST_CHECKED:BST_UNCHECKED);
			SendDlgItemMessage(hwndDlg, IDC_SSSPIN2, UDM_SETPOS, 0, MAKELONG((PopUpTime), 0));
			CheckDlgButton(hwndDlg, IDC_MENU, (MenuItem == 1) ? BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_TTB, (TTBButtons == 1) ? BST_CHECKED:BST_UNCHECKED);
			break;
		}
		break;
	case WM_NOTIFY:
	case WM_DESTROY:
		break;
	}
	return FALSE;
}