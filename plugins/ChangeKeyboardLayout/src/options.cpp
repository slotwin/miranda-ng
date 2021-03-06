#include "commonheaders.h"

INT_PTR CALLBACK DlgMainProcOptions(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	static BOOL MainDialogLock = FALSE;
	LPTSTR ptszGenLay, ptszMemLay, ptszFormLay, ptszShortNameLay;
	LPSTR pszNameLay;
	BYTE i;

	switch (uiMessage) {
	case WM_INITDIALOG:
		MainDialogLock = TRUE;
		TranslateDialogDefault(hWnd);
			
		//������� �������
		// ��������� ������� � �������� ���, ����� ������� ������
		SendDlgItemMessage(hWnd, IDC_HOTKEY_LAYOUT, HKM_SETRULES, 0xFF, 0);
		SendDlgItemMessage(hWnd, IDC_HOTKEY_LAYOUT2, HKM_SETRULES, 0xFF, 0);
		SendDlgItemMessage(hWnd, IDC_HOTKEY_CASE, HKM_SETRULES, 0xFF, 0);
			
		//���������� ����������� �������
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_SHIFT, BM_SETCHECK, (moOptions.dwHotkey_Layout&0x00000100), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_CTRL, BM_SETCHECK, (moOptions.dwHotkey_Layout&0x00000200), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_ALT, BM_SETCHECK, (moOptions.dwHotkey_Layout&0x00000400), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_WIN, BM_SETCHECK, (moOptions.dwHotkey_Layout&0x00000800), 0);

		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_SHIFT, BM_SETCHECK, (moOptions.dwHotkey_Layout2&0x00000100), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_CTRL, BM_SETCHECK, (moOptions.dwHotkey_Layout2&0x00000200), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_ALT, BM_SETCHECK, (moOptions.dwHotkey_Layout2&0x00000400), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_WIN, BM_SETCHECK, (moOptions.dwHotkey_Layout2&0x00000800), 0);

		SendDlgItemMessage(hWnd, IDC_CHECK_CASE_SHIFT, BM_SETCHECK, (moOptions.dwHotkey_Case&0x00000100), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_CASE_CTRL, BM_SETCHECK, (moOptions.dwHotkey_Case&0x00000200), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_CASE_ALT, BM_SETCHECK, (moOptions.dwHotkey_Case&0x00000400), 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_CASE_WIN, BM_SETCHECK, (moOptions.dwHotkey_Case&0x00000800), 0);

		//���������� ������ �� ������
		SendDlgItemMessage(hWnd, IDC_HOTKEY_LAYOUT, HKM_SETHOTKEY, moOptions.dwHotkey_Layout&0x000000FF, 0);
		SendDlgItemMessage(hWnd, IDC_HOTKEY_LAYOUT2, HKM_SETHOTKEY, moOptions.dwHotkey_Layout2&0x000000FF, 0);
		SendDlgItemMessage(hWnd, IDC_HOTKEY_CASE, HKM_SETHOTKEY, moOptions.dwHotkey_Case&0x000000FF, 0);
			
		//��������� �����
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_MODE, BM_SETCHECK, moOptions.CurrentWordLayout, 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_MODE2, BM_SETCHECK, moOptions.CurrentWordLayout2, 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_CASE_MODE, BM_SETCHECK, moOptions.CurrentWordCase, 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_TWOWAY, BM_SETCHECK, moOptions.TwoWay, 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_SYSTEMLAYOUT, BM_SETCHECK, moOptions.ChangeSystemLayout, 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_CLIPBOARD, BM_SETCHECK, moOptions.CopyToClipboard, 0);
		SendDlgItemMessage(hWnd, IDC_CHECK_POPUP, BM_SETCHECK, moOptions.ShowPopup, 0);

		//����� ��������� CapsLock
		switch(moOptions.bCaseOperations) {
		case 1:
			SendDlgItemMessage(hWnd, IDC_RADIO_OFFCAPS, BM_SETCHECK, 1, 0);
			break;
		case 2:
			SendDlgItemMessage(hWnd, IDC_RADIO_IGNORECAPS, BM_SETCHECK, 1, 0);
			break;
		default:
			SendDlgItemMessage(hWnd, IDC_RADIO_INVERTCAPS, BM_SETCHECK, 1, 0);
			break;
		}

		// ���������� ������ ������.������
		ptszMemLay = ptszLayStrings[0];
		SendDlgItemMessage(hWnd, IDC_EDIT_EXAMPLE, WM_SETTEXT, 0, (LPARAM)ptszMemLay);
		ptszShortNameLay = GetShortNameOfLayout(hklLayouts[0]);
		SendDlgItemMessage(hWnd, IDC_STATIC_EXAMPLE, WM_SETTEXT, 0, (LPARAM)ptszShortNameLay);
		mir_free(ptszShortNameLay);

		// ��������� ��������� � �������� �����������
		for(i = 0; i < bLayNum; i++) {
			ptszShortNameLay = GetShortNameOfLayout(hklLayouts[i]);				
			SendDlgItemMessage(hWnd, IDC_COMBO_LANG, CB_ADDSTRING, 0, (LPARAM)ptszShortNameLay);
			mir_free(ptszShortNameLay);
		}
		//���������� ������ ��������� � ������
		SendDlgItemMessage(hWnd, IDC_COMBO_LANG, CB_SETCURSEL, 0, 0);
		ptszMemLay = ptszLayStrings[0];
		SendDlgItemMessage(hWnd, IDC_EDIT_SET, WM_SETTEXT, 0, (LPARAM)ptszMemLay);
			
		if (bLayNum != 2) EnableWindow(GetDlgItem(hWnd, IDC_CHECK_TWOWAY), FALSE);		
		MainDialogLock = FALSE;
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_HOTKEY_LAYOUT:
		case IDC_HOTKEY_LAYOUT2:
		case IDC_HOTKEY_CASE:			
			if ((HIWORD(wParam) == EN_CHANGE))
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			break;

		case IDC_CHECK_DETECT:
		case IDC_CHECK_LAYOUT_MODE:
		case IDC_CHECK_LAYOUT_MODE2:
		case IDC_CHECK_CASE_MODE:
		case IDC_CHECK_TWOWAY:
		case IDC_CHECK_SYSTEMLAYOUT:
		case IDC_CHECK_POPUP:
		case IDC_CHECK_CLIPBOARD:
		case IDC_CHECK_LAYOUT_SHIFT:
		case IDC_CHECK_LAYOUT_ALT:
		case IDC_CHECK_LAYOUT_CTRL:
		case IDC_CHECK_LAYOUT_WIN:
		case IDC_CHECK_LAYOUT2_SHIFT:
		case IDC_CHECK_LAYOUT2_ALT:
		case IDC_CHECK_LAYOUT2_CTRL:
		case IDC_CHECK_LAYOUT2_WIN:
		case IDC_CHECK_CASE_SHIFT:
		case IDC_CHECK_CASE_ALT:
		case IDC_CHECK_CASE_CTRL:
		case IDC_CHECK_CASE_WIN:
		case IDC_RADIO_IGNORECAPS:
		case IDC_RADIO_INVERTCAPS:
		case IDC_RADIO_OFFCAPS:
			if ((HIWORD(wParam) == BN_CLICKED))
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			break;

		case IDC_COMBO_LANG:
			if ((HIWORD(wParam) == CBN_SELCHANGE)) {
				MainDialogLock = TRUE;
				ptszMemLay = ptszLayStrings[SendDlgItemMessage(hWnd, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0)];
				SendDlgItemMessage(hWnd, IDC_EDIT_SET, WM_SETTEXT, 0, (LPARAM)ptszMemLay);
				MainDialogLock = FALSE;
			}
			break;

		case IDC_EDIT_SET:
			if ((HIWORD(wParam) == EN_CHANGE) && (!MainDialogLock))
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			break;

		case IDC_BUTTON_DEFAULT:
			if ((HIWORD(wParam) == BN_CLICKED )) {
				ptszGenLay = GenerateLayoutString(hklLayouts[SendDlgItemMessage(hWnd, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0)]);
				SendDlgItemMessage(hWnd, IDC_EDIT_SET, WM_SETTEXT, 0, (LPARAM)ptszGenLay);
				mir_free(ptszGenLay);
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			}
			break;
		}
		break;	

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0: 
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				// ��������� ������
				moOptions.dwHotkey_Layout = SendDlgItemMessage(hWnd, IDC_HOTKEY_LAYOUT, HKM_GETHOTKEY, 0, 0);
				moOptions.dwHotkey_Layout2 = SendDlgItemMessage(hWnd, IDC_HOTKEY_LAYOUT2, HKM_GETHOTKEY, 0, 0);
				moOptions.dwHotkey_Case = SendDlgItemMessage(hWnd, IDC_HOTKEY_CASE, HKM_GETHOTKEY, 0, 0);							

				//������� � �������� ����������� �������
				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_SHIFT, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout |= 0x00000100;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_CTRL, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout |= 0x00000200;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_ALT, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout |= 0x00000400;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_WIN, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout |= 0x00000800;

				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_SHIFT, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout2 |= 0x00000100;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_CTRL, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout2 |= 0x00000200;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_ALT, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout2 |= 0x00000400;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT2_WIN, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Layout2 |= 0x00000800;

				if (SendDlgItemMessage(hWnd, IDC_CHECK_CASE_SHIFT, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Case |= 0x00000100;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_CASE_CTRL, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Case |= 0x00000200;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_CASE_ALT, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Case |= 0x00000400;
				if (SendDlgItemMessage(hWnd, IDC_CHECK_CASE_WIN, BM_GETCHECK, 0, 0)) 
					moOptions.dwHotkey_Case |= 0x00000800;


				//������ �����
				moOptions.CurrentWordLayout = SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_MODE, BM_GETCHECK, 0, 0);
				moOptions.CurrentWordLayout2 = SendDlgItemMessage(hWnd, IDC_CHECK_LAYOUT_MODE2, BM_GETCHECK, 0, 0);
				moOptions.CurrentWordCase = SendDlgItemMessage(hWnd, IDC_CHECK_CASE_MODE, BM_GETCHECK, 0, 0);
				moOptions.TwoWay = SendDlgItemMessage(hWnd, IDC_CHECK_TWOWAY, BM_GETCHECK, 0, 0);
				moOptions.ChangeSystemLayout = SendDlgItemMessage(hWnd, IDC_CHECK_SYSTEMLAYOUT, BM_GETCHECK, 0, 0);
				moOptions.CopyToClipboard = SendDlgItemMessage(hWnd, IDC_CHECK_CLIPBOARD, BM_GETCHECK, 0, 0);
				moOptions.ShowPopup = SendDlgItemMessage(hWnd, IDC_CHECK_POPUP, BM_GETCHECK, 0, 0);

				// CapsLock
				if (SendDlgItemMessage(hWnd, IDC_RADIO_OFFCAPS, BM_GETCHECK, 0, 0) == BST_CHECKED)
					moOptions.bCaseOperations = 1;
				else if (SendDlgItemMessage(hWnd, IDC_RADIO_IGNORECAPS, BM_GETCHECK, 0, 0) == BST_CHECKED)
					moOptions.bCaseOperations = 2;
				else moOptions.bCaseOperations = 0;								

				WriteMainOptions();

				ptszFormLay = (LPTSTR)mir_alloc(MaxTextSize*sizeof(TCHAR));
				SendDlgItemMessage(hWnd, IDC_EDIT_SET, WM_GETTEXT, (WPARAM) MaxTextSize, (LPARAM)ptszFormLay);
				i = SendDlgItemMessage(hWnd, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0);
				ptszMemLay = ptszLayStrings[i];
				if (_tcscmp(ptszMemLay, ptszFormLay) != 0) {
					_tcscpy(ptszMemLay, ptszFormLay);
					ptszGenLay = GenerateLayoutString(hklLayouts[i]);
					pszNameLay = GetNameOfLayout(hklLayouts[i]);

					if (_tcscmp(ptszMemLay, ptszGenLay) != 0)
						db_set_ts(NULL, ModuleName, pszNameLay, ptszMemLay);
					else
						db_unset(NULL, ModuleName, pszNameLay);

					mir_free(pszNameLay);
					mir_free(ptszGenLay);
				}
				mir_free(ptszFormLay);							

				ptszMemLay = ptszLayStrings[0];
				SendDlgItemMessage(hWnd, IDC_EDIT_EXAMPLE, WM_SETTEXT, 0, (LPARAM)ptszMemLay);

				UnhookWindowsHookEx(kbHook_All);
				kbHook_All = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)Keyboard_Hook, NULL, GetCurrentThreadId());
			}
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK DlgPopupsProcOptions(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	static BOOL PopupDialogLock = FALSE;
	LPTSTR ptszPopupPreviewText;
	DWORD dwTimeOut;

	switch (uiMessage) {
	case WM_INITDIALOG:
		PopupDialogLock = TRUE;
		TranslateDialogDefault(hWnd);
		poOptionsTemp = poOptions;

		//�����
		SendDlgItemMessage(hWnd, IDC_CUSTOM_BACK, CPM_SETCOLOUR, 0, poOptionsTemp.crBackColour);
		SendDlgItemMessage(hWnd, IDC_CUSTOM_TEXT, CPM_SETCOLOUR, 0, poOptionsTemp.crTextColour);
		CheckDlgButton(hWnd, IDC_RADIO_COLOURS_POPUP, poOptionsTemp.bColourType == PPC_POPUP);
		CheckDlgButton(hWnd, IDC_RADIO_COLOURS_WINDOWS, poOptionsTemp.bColourType == PPC_WINDOWS);
		CheckDlgButton(hWnd, IDC_RADIO_COLOURS_CUSTOM, poOptionsTemp.bColourType == PPC_CUSTOM);
		EnableWindow(GetDlgItem(hWnd, IDC_CUSTOM_BACK), poOptionsTemp.bColourType == PPC_CUSTOM);
		EnableWindow(GetDlgItem(hWnd, IDC_CUSTOM_TEXT), poOptionsTemp.bColourType == PPC_CUSTOM);

		// �������
		CheckDlgButton(hWnd, IDC_RADIO_TIMEOUT_POPUP, poOptionsTemp.bTimeoutType == PPT_POPUP);
		CheckDlgButton(hWnd, IDC_RADIO_TIMEOUT_PERMANENT, poOptionsTemp.bTimeoutType == PPT_PERMANENT);
		CheckDlgButton(hWnd, IDC_RADIO_TIMEOUT_CUSTOM, poOptionsTemp.bTimeoutType == PPT_CUSTOM);			
		SetDlgItemInt(hWnd, IDC_EDIT_TIMEOUT, poOptionsTemp.bTimeout, FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_EDIT_TIMEOUT), poOptionsTemp.bTimeoutType == PPT_CUSTOM);

		// ���� �����
		CheckDlgButton(hWnd, IDC_RADIO_LEFT_CLIPBOARD, poOptionsTemp.bLeftClick == 0);
		CheckDlgButton(hWnd, IDC_RADIO_LEFT_DISMISS, poOptionsTemp.bLeftClick == 1);
		// ���� ������
		CheckDlgButton(hWnd, IDC_RADIO_RIGHT_CLIPBOARD, poOptionsTemp.bRightClick == 0);
		CheckDlgButton(hWnd, IDC_RADIO_RIGHT_DISMISS, poOptionsTemp.bRightClick == 1);
		PopupDialogLock = FALSE;
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RADIO_COLOURS_POPUP:	
		case IDC_RADIO_COLOURS_WINDOWS:
		case IDC_RADIO_COLOURS_CUSTOM:
			if ((HIWORD(wParam) == BN_CLICKED)) {
				if (IsDlgButtonChecked(hWnd, IDC_RADIO_COLOURS_POPUP))
					poOptionsTemp.bColourType = PPC_POPUP;
				else if (IsDlgButtonChecked(hWnd, IDC_RADIO_COLOURS_WINDOWS))
					poOptionsTemp.bColourType = PPC_WINDOWS;
				else if (IsDlgButtonChecked(hWnd, IDC_RADIO_COLOURS_CUSTOM))
					poOptionsTemp.bColourType = PPC_CUSTOM;

				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOM_BACK), poOptionsTemp.bColourType == PPC_CUSTOM);
				EnableWindow(GetDlgItem(hWnd, IDC_CUSTOM_TEXT), poOptionsTemp.bColourType == PPC_CUSTOM);
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_RADIO_TIMEOUT_POPUP:
		case IDC_RADIO_TIMEOUT_PERMANENT:
		case IDC_RADIO_TIMEOUT_CUSTOM:
			if ((HIWORD(wParam) == BN_CLICKED)) {
				if (IsDlgButtonChecked(hWnd, IDC_RADIO_TIMEOUT_POPUP))
					poOptionsTemp.bTimeoutType = PPT_POPUP;
				else if (IsDlgButtonChecked(hWnd, IDC_RADIO_TIMEOUT_PERMANENT))
					poOptionsTemp.bTimeoutType = PPT_PERMANENT;
				if (IsDlgButtonChecked(hWnd, IDC_RADIO_TIMEOUT_CUSTOM))
					poOptionsTemp.bTimeoutType = PPT_CUSTOM;

				EnableWindow(GetDlgItem(hWnd, IDC_EDIT_TIMEOUT), poOptionsTemp.bTimeoutType == PPT_CUSTOM);
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_RADIO_LEFT_CLIPBOARD:
		case IDC_RADIO_LEFT_DISMISS:
			if ((HIWORD(wParam) == BN_CLICKED)) {
				if (IsDlgButtonChecked(hWnd, IDC_RADIO_LEFT_CLIPBOARD))
					poOptionsTemp.bLeftClick = 0;
				else if (IsDlgButtonChecked(hWnd, IDC_RADIO_LEFT_DISMISS))
					poOptionsTemp.bLeftClick = 1;
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_RADIO_RIGHT_CLIPBOARD:
		case IDC_RADIO_RIGHT_DISMISS:
			if ((HIWORD(wParam) == BN_CLICKED)) {
				if (IsDlgButtonChecked(hWnd, IDC_RADIO_RIGHT_CLIPBOARD))
					poOptionsTemp.bRightClick = 0;
				else if (IsDlgButtonChecked(hWnd, IDC_RADIO_RIGHT_DISMISS))
					poOptionsTemp.bRightClick = 1;
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_CUSTOM_BACK:
		case IDC_CUSTOM_TEXT:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				poOptionsTemp.crBackColour = SendDlgItemMessage(hWnd, IDC_CUSTOM_BACK, CPM_GETCOLOUR, 0, 0);
				poOptionsTemp.crTextColour = SendDlgItemMessage(hWnd, IDC_CUSTOM_TEXT, CPM_GETCOLOUR, 0, 0);
				SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);							
			}
			break;

		case IDC_EDIT_TIMEOUT:
			if (HIWORD(wParam) == EN_CHANGE) {
				dwTimeOut = GetDlgItemInt(hWnd, IDC_EDIT_TIMEOUT, NULL, FALSE);
				if (dwTimeOut>255)
					poOptionsTemp.bTimeout = 255;
				else 
					poOptionsTemp.bTimeout = dwTimeOut;

				if (!PopupDialogLock)
					SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_BUTTON_PREVIEW:
			if ((HIWORD(wParam) == BN_CLICKED )) {
				ptszPopupPreviewText = (LPTSTR)mir_alloc(MaxTextSize*sizeof(TCHAR));

				POPUPDATAT_V2 pdtData = { 0 };
				pdtData.cbSize = sizeof(pdtData);
				_tcsncpy(pdtData.lptzContactName, TranslateT(ModuleName), MAX_CONTACTNAME);
				_tcsncpy(pdtData.lptzText, _T("Ghbdtn? rfr ltkf&"), MAX_SECONDLINE);

				switch(poOptionsTemp.bColourType) {
				case PPC_POPUP:
					pdtData.colorBack = pdtData.colorText = 0;
					break;
				case PPC_WINDOWS:
					pdtData.colorBack = GetSysColor(COLOR_BTNFACE);
					pdtData.colorText = GetSysColor(COLOR_WINDOWTEXT);
					break;
				case PPC_CUSTOM:
					pdtData.colorBack = poOptionsTemp.crBackColour;
					pdtData.colorText = poOptionsTemp.crTextColour;
					break;
				}						
				
				switch(poOptionsTemp.bTimeoutType) {
				case PPT_POPUP:
					pdtData.iSeconds = 0;
					break;
				case PPT_PERMANENT:
					pdtData.iSeconds = -1;
					break;
				case PPC_CUSTOM:
					pdtData.iSeconds = poOptionsTemp.bTimeout;
					break;
				}
				_tcscpy(ptszPopupPreviewText, pdtData.lptzText);
				pdtData.PluginData = ptszPopupPreviewText;
				pdtData.lchIcon = hPopupIcon;
				poOptions.paActions[0].lchIcon = hCopyIcon;
				pdtData.lpActions = poOptions.paActions;
				pdtData.actionCount = 1;
				pdtData.PluginWindowProc = (WNDPROC)CKLPopupDlgProc;

				if ( CallService(MS_POPUP_ADDPOPUPT, (WPARAM) &pdtData, APF_NEWDATA) < 0)
					mir_free(ptszPopupPreviewText);
			}
			break;
		}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				poOptions = poOptionsTemp;
				WritePopupOptions();
			}
		}
		break;
	}
	return FALSE;
}
