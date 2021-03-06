// History Linklist Plus
// Copyright (C) 2010 Thomas Wendel, gureedo
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#include "linklist.h"

extern HINSTANCE hInst;
extern HANDLE hWindowList;
extern HCURSOR splitCursor;

MYCOLOURSET colourSet;
LISTOPTIONS options;
/*
MainDlgProc handles messages to the main dialog box
*/
INT_PTR WINAPI MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	ENLINK* pENLink;
	DIALOGPARAM *DlgParam;
	HMENU listMenu = GetMenu(hDlg);
	
	DlgParam = (DIALOGPARAM *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	switch ( msg )
	{
		case WM_INITDIALOG:
		{
			MCONTACT hContact;
			TCHAR title[256];
			TCHAR filter[FILTERTEXT];
			RECT rc;
			POINT pt;
			
			SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
			DlgParam = (DIALOGPARAM *)lParam;
			TranslateDialogDefault(hDlg);
			TranslateMenu(listMenu);

			if ( db_get_b(NULL, LINKLIST_MODULE, LINKLIST_SAVESPECIAL, 0x00) == 0x00 )
				hContact = NULL;
			else 
				hContact = DlgParam->hContact;
			
			if ( db_get_b(hContact, LINKLIST_MODULE, LINKLIST_FIRST, 0) == 0 )
			{
				// First use of this plugin! Set default size!
				db_set_dw(hContact, LINKLIST_MODULE, "LinklistWidth", 400);
				db_set_dw(hContact, LINKLIST_MODULE, "LinklistHeight", 450);
				db_set_dw(hContact, LINKLIST_MODULE, "LinklistX", 0);
				db_set_dw(hContact, LINKLIST_MODULE, "LinklistY", 0);
				
				db_set_b(hContact, LINKLIST_MODULE, LINKLIST_FIRST, 1);
			}

			DlgParam->splitterPosNew = (int)db_get_dw(hContact, LINKLIST_MODULE, LINKLIST_SPLITPOS, -1);

			GetWindowRect(GetDlgItem(hDlg, IDC_MAIN), &rc);
			DlgParam->minSize.cx = rc.right - rc.left;
            DlgParam->minSize.cy = rc.bottom - rc.top;

            GetWindowRect(GetDlgItem(hDlg, IDC_SPLITTER), &rc);
            pt.y = (rc.top + rc.bottom) / 2;
			pt.x = 0;
			ScreenToClient(hDlg, &pt);
			
			DlgParam->splitterPosOld = rc.bottom - 20 - pt.y;
			if(DlgParam->splitterPosNew == -1)
				DlgParam->splitterPosNew = DlgParam->splitterPosOld;
			
			Utils_RestoreWindowPosition(hDlg, hContact, LINKLIST_MODULE, "Linklist");

			SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)LoadIcon(hInst, MAKEINTRESOURCE(IDI_LINKLISTICON))); 
			WindowList_Add(hWindowList, hDlg, DlgParam->hContact);
			mir_sntprintf(title, _countof(title), _T("%s [%s]"), TranslateT("Linklist Plugin"), (LPCTSTR)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)DlgParam->hContact, GCDNF_TCHAR));
			SetWindowText(hDlg, title);
			GetFilterText(listMenu, filter, _countof(filter));
			SetDlgItemText(hDlg, IDC_STATUS, filter);
			
			mir_subclassWindow(GetDlgItem(hDlg, IDC_SPLITTER), SplitterProc);
			
			SendDlgItemMessage( hDlg, IDC_MAIN, EM_SETEVENTMASK, 0, (LPARAM)ENM_LINK );
			SendDlgItemMessage( hDlg, IDC_MAIN, EM_AUTOURLDETECT, TRUE, 0 );
			// This is used in srmm... and I think he knew what he did... :)
			SendDlgItemMessage(hDlg, IDC_MAIN, EM_LIMITTEXT, (WPARAM)-1, 0);

			WriteLinkList( hDlg, WLL_ALL, (LISTELEMENT *)DlgParam->listStart, NULL, 0);

			return TRUE;
		}

		// open browser an load url if link is pressed
		// found at
		// http://www.tech-archive.net/Archive/Development/microsoft.public.win32.programmer.ui/2004-03/0133.html
		//
		// Popup menu on right mouse button is mainly taken from the miranda 
		// send/receive messaging plugin.
		case WM_NOTIFY:
		{  
			LPNMHDR lpNmhdr;
	
			lpNmhdr = (LPNMHDR)lParam;
			if ( lpNmhdr->code == EN_LINK )
			{ 
				LPTSTR link;
				char shEvent[10+1];
				BYTE openNewWindow, mouseEvent;
				
				ZeroMemory(shEvent, _countof(shEvent));

				pENLink = (ENLINK*)lpNmhdr;

				mouseEvent = db_get_b(NULL, LINKLIST_MODULE, LINKLIST_MOUSE_EVENT, 0xFF);

				if ( pENLink->msg == WM_MOUSEMOVE && mouseEvent == 0x01 )
				{
					CopyMemory(&DlgParam->chrg, &pENLink->chrg, sizeof(CHARRANGE));
					SendDlgItemMessage(hDlg, IDC_MAIN, EM_EXSETSEL, 0, (LPARAM)&pENLink->chrg);  
					WriteMessage(hDlg, DlgParam->listStart, SendDlgItemMessage(hDlg, IDC_MAIN, EM_LINEFROMCHAR, -1, 0));
				}
				else if ( pENLink->msg == WM_LBUTTONUP )
				{ 
					link = (LPTSTR)malloc( (pENLink->chrg.cpMax-pENLink->chrg.cpMin+2)*sizeof(TCHAR));
					SendDlgItemMessage(hDlg, IDC_MAIN, EM_EXSETSEL, 0, (LPARAM)(&pENLink->chrg)); 
					SendDlgItemMessage(hDlg, IDC_MAIN, EM_GETSELTEXT, 0, (LPARAM)link);
					
					if ( _tcsstr(link, _T("mailto:")) != NULL )
						ShellExecute(HWND_TOP, NULL, link, NULL, NULL, SW_SHOWNORMAL); 
					else
					{
						openNewWindow = db_get_b(NULL, LINKLIST_MODULE, LINKLIST_OPEN_WINDOW, 0xFF);
						if ( openNewWindow == 0xFF )
							openNewWindow = 0;
	
						CallService(MS_UTILS_OPENURL, openNewWindow, (LPARAM)link);
					}
					free(link);
				} 
				else if ( pENLink->msg == WM_RBUTTONDOWN )
				{ 
					HMENU hPopup, hSubMenu;
					POINT pt;
					hPopup = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU2));
					hSubMenu = GetSubMenu(hPopup, 0);

					// Disable Menuoption if "mouse over" events are active
					mouseEvent = db_get_b(NULL, LINKLIST_MODULE, LINKLIST_MOUSE_EVENT, 0xFF);
					if (mouseEvent == 0x01 )
						EnableMenuItem(hSubMenu, IDM_SHOWMESSAGE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
					
					TranslateMenu(hSubMenu);
					
					link = (LPTSTR)malloc( (pENLink->chrg.cpMax-pENLink->chrg.cpMin+2)*sizeof(TCHAR));
					SendDlgItemMessage(hDlg, IDC_MAIN, EM_EXSETSEL, 0, (LPARAM)(&(pENLink->chrg))); 
					SendDlgItemMessage(hDlg, IDC_MAIN, EM_GETSELTEXT, 0, (LPARAM)link);
					
					pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
	                pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
					ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
				
					switch ( TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hDlg, NULL)) 
					{
						case IDM_LINK_OPEN:
						{
							if ( _tcsstr(link, _T("mailto:")) != NULL )
								ShellExecute(HWND_TOP, NULL, link, NULL, NULL, SW_SHOWNORMAL); 
							else
								CallService(MS_UTILS_OPENURL, 0, (LPARAM)link);
	
							break;
						}
						case IDM_LINK_OPENNEW:
						{
							if ( _tcsstr(link, _T("mailto:")) != NULL )
								ShellExecute(HWND_TOP, NULL, link, NULL, NULL, SW_SHOWNORMAL); 
							else
								CallService(MS_UTILS_OPENURL, 1, (LPARAM)link);

							break;
						}
						case IDM_LINK_COPY:
						{
							size_t dataLen;
							HGLOBAL hData;
							if ( !OpenClipboard(hDlg))
								break;
							EmptyClipboard();

							dataLen = (_tcslen(link)+1)*sizeof(TCHAR);
							hData = GlobalAlloc(GMEM_MOVEABLE, dataLen);
							_tcscpy_s((LPTSTR)GlobalLock(hData), dataLen/2, link);
							GlobalUnlock(hData);
							SetClipboardData(CF_TEXT, hData);
							CloseClipboard();
						    break;
						}
						case IDM_SHOWMESSAGE:
						{
							WriteMessage(hDlg, DlgParam->listStart, SendDlgItemMessage(hDlg, IDC_MAIN, EM_LINEFROMCHAR, -1, 0));
						    break;
						}
					}
					free(link);
					DestroyMenu(hPopup);
				} 
			}
			break;
		}
		
		case WM_COMMAND:
		{
			TCHAR filter[40];

			// open Search Box
			if ( wParam == IDM_SEARCH )
			{

				HWND hWndSearchDlg;
				if ( DlgParam != 0 )
				{	
					hWndSearchDlg = CreateDialogParam( hInst, MAKEINTRESOURCE(IDD_SEARCH_DLG), hDlg, SearchDlgProc, (LPARAM)DlgParam);
					EnableMenuItem(listMenu, IDM_SEARCH, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
					ShowWindow(hWndSearchDlg, SW_SHOW);
					SetFocus(GetDlgItem(hWndSearchDlg, IDC_SEARCHSTRING));
				}
				
				break;
			}
			// clear search results
			else if ( wParam == IDM_CLEARSEARCH )
			{	
				GetFilterText(listMenu, filter, _countof(filter));
				SetDlgItemText(hDlg, IDC_STATUS, filter);
				SetDlgItemText(hDlg, IDC_MAIN, _T(""));
				WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
			}
			// save button
			else if ( wParam == IDM_SAVE )
			{
				SaveEditAsStream(hDlg);
				SetFocus(GetDlgItem( hDlg, IDC_MAIN ));
				break;
			}
			// Esc or Close pressed
			else if ( wParam == IDCANCEL || wParam == IDM_CLOSE )
			{
				SendMessage(hDlg, WM_CLOSE, 0, 0);
				break;
			}
			
			// view only incoming messages
			else if ( wParam == IDM_DIR_IN )
			{
				GetFilterText(listMenu, filter, _countof(filter));
				SetDlgItemText(hDlg, IDC_STATUS, filter);
				
				// not possible if search dialog is open
				if ( !(GetMenuState(listMenu, IDM_SEARCH, MF_BYCOMMAND) & MF_DISABLED))
				{
					SetDlgItemText(hDlg, IDC_MAIN, _T(""));

					if ( GetMenuState(listMenu, IDM_DIR_IN, MF_BYCOMMAND) == MF_CHECKED )
					{
						CheckMenuItem(listMenu, IDM_DIR_IN, MF_UNCHECKED);
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
					}
					else
					{
						CheckMenuItem(listMenu, IDM_DIR_IN, MF_CHECKED);
						CheckMenuItem(listMenu, IDM_DIR_OUT, MF_UNCHECKED);
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
					}
					
					GetFilterText(GetMenu(hDlg), filter, _countof(filter));
					SetDlgItemText(hDlg, IDC_STATUS, filter);
				}
				break;
			}			
			
			// view only outgoing messages
			else if ( wParam == IDM_DIR_OUT )
			{
				GetFilterText(listMenu, filter, _countof(filter));
				SetDlgItemText(hDlg, IDC_STATUS, filter);

				// not possible if search dialog is open
				if ( !(GetMenuState(listMenu, IDM_SEARCH, MF_BYCOMMAND) & MF_DISABLED))
				{
					SetDlgItemText(hDlg, IDC_MAIN, _T(""));
					if ( GetMenuState(listMenu, IDM_DIR_OUT, MF_BYCOMMAND) == MF_CHECKED )
					{
						CheckMenuItem(listMenu, IDM_DIR_OUT, MF_UNCHECKED);		
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
					}
					else
					{
						CheckMenuItem(listMenu, IDM_DIR_OUT, MF_CHECKED);
						CheckMenuItem(listMenu, IDM_DIR_IN, MF_UNCHECKED);
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
					}

					GetFilterText(listMenu, filter, _countof(filter));
					SetDlgItemText(hDlg, IDC_STATUS, filter);
				}
				break;
			}	
			
			// view only e-mailaddresses
			else if ( wParam == IDM_TYPE_WEB )
			{
				GetFilterText(listMenu, filter, _countof(filter));
				SetDlgItemText(hDlg, IDC_STATUS, filter);

				// not possible if search dialog is open
				if ( !(GetMenuState(listMenu, IDM_SEARCH, MF_BYCOMMAND) & MF_DISABLED))
				{
					SetDlgItemText(hDlg, IDC_MAIN, _T(""));
					if ( GetMenuState(listMenu, IDM_TYPE_WEB, MF_BYCOMMAND) == MF_CHECKED )
					{
						CheckMenuItem(listMenu, IDM_TYPE_WEB, MF_UNCHECKED);		
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
					}
					else
					{
						CheckMenuItem(listMenu, IDM_TYPE_WEB, MF_CHECKED);
						CheckMenuItem(listMenu, IDM_TYPE_MAIL, MF_UNCHECKED);
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
					}

					GetFilterText(listMenu, filter, _countof(filter));
					SetDlgItemText(hDlg, IDC_STATUS, filter);
				}
				break;
			}	
			
			// view only URLs
			else if ( wParam == IDM_TYPE_MAIL )
			{
				// not possible if search dialog is open
				if ( !(GetMenuState(listMenu, IDM_SEARCH, MF_BYCOMMAND) & MF_DISABLED))
				{
					SetDlgItemText(hDlg, IDC_MAIN, _T(""));
					if ( GetMenuState(listMenu, IDM_TYPE_MAIL, MF_BYCOMMAND) == MF_CHECKED )
					{
						CheckMenuItem(listMenu, IDM_TYPE_MAIL, MF_UNCHECKED);		
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);				
					}
					else
					{
						CheckMenuItem(listMenu, IDM_TYPE_MAIL, MF_CHECKED);
						CheckMenuItem(listMenu, IDM_TYPE_WEB, MF_UNCHECKED);
						WriteLinkList( hDlg, GetFlags(listMenu), DlgParam->listStart, NULL, 0);
					}

					GetFilterText(listMenu, filter, _countof(filter));
					SetDlgItemText(hDlg, IDC_STATUS, filter);
				}
				break;
			}
			

			break;
		}

		// Taken from srmm.
		// Btw: The longer I searched the source of this plugin
		// to learn how things work, the more I became a fan of
		// the programmer! 
		case WM_GETMINMAXINFO:
        {
			MINMAXINFO *mmi = (MINMAXINFO *)lParam;
			RECT rcWindow, rcMain;
			GetWindowRect(hDlg, &rcWindow);
            GetWindowRect(GetDlgItem(hDlg, IDC_MAIN), &rcMain);
            mmi->ptMinTrackSize.x = rcWindow.right - rcWindow.left - ((rcMain.right - rcMain.left) - DlgParam->minSize.cx);
            mmi->ptMinTrackSize.y = rcWindow.bottom - rcWindow.top - ((rcMain.bottom - rcMain.top) - DlgParam->minSize.cy);
            return 0;
        }

		case WM_SIZE:
		{
			UTILRESIZEDIALOG urd = {0};
		
			urd.cbSize = sizeof(urd);
			urd.hwndDlg = hDlg;
			urd.hInstance = hInst;
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_MAIN_DLG);
			urd.pfnResizer = LinklistResizer;
			urd.lParam = (LPARAM)DlgParam;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
			
			// To get some scrollbars if needed...
			RedrawWindow(GetDlgItem(hDlg, IDC_MAIN), NULL, NULL, RDW_INVALIDATE);
			RedrawWindow(GetDlgItem(hDlg, IDC_MESSAGE), NULL, NULL, RDW_INVALIDATE);
			break;
		}

		case DM_LINKSPLITTER:
		{
			POINT pt;
            RECT rc;
			int splitPosOld;

			GetClientRect(hDlg, &rc);
			pt.x = 0;
			pt.y = wParam;
			ScreenToClient(hDlg, &pt);

			splitPosOld = DlgParam->splitterPosNew;
			DlgParam->splitterPosNew = rc.bottom - pt.y;
			
			GetWindowRect(GetDlgItem(hDlg, IDC_MESSAGE), &rc);
			if (rc.bottom - rc.top + (DlgParam->splitterPosNew - splitPosOld) < 0)
				DlgParam->splitterPosNew = splitPosOld + 0 - (rc.bottom - rc.top);

			GetWindowRect(GetDlgItem(hDlg, IDC_MAIN), &rc);
			if (rc.bottom - rc.top - (DlgParam->splitterPosNew - splitPosOld) < DlgParam->minSize.cy)
				DlgParam->splitterPosNew = splitPosOld - DlgParam->minSize.cy + (rc.bottom - rc.top);
	
            SendMessage(hDlg, WM_SIZE, 0, 0);
			break;
		}

		case WM_CLOSE:
		{
			DestroyWindow(hDlg);
			break;
		}

		case WM_DESTROY:
		{
			MCONTACT hContact;
			if ( db_get_b(NULL, LINKLIST_MODULE, LINKLIST_SAVESPECIAL, 0x00) == 0x00 )
				hContact = NULL;
			else
				hContact = DlgParam->hContact;
			Utils_SaveWindowPosition(hDlg, hContact, LINKLIST_MODULE, "Linklist");
			db_set_dw(NULL, LINKLIST_MODULE, LINKLIST_SPLITPOS, DlgParam->splitterPosNew);
			RemoveList(DlgParam->listStart);
			free(DlgParam);
			// Remove entry from Window list
			WindowList_Remove(hWindowList, hDlg);
			break;
		}
		
	}
	return FALSE;
}

/*
This function handles the search dialog messages
*/
INT_PTR WINAPI SearchDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	HWND hListDlg;
	DIALOGPARAM *DlgParam;
	
	DlgParam = (DIALOGPARAM *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	switch( msg )
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hDlg);
			SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
			SetWindowText(hDlg, TXT_SEARCH);
			SendDlgItemMessage(hDlg, IDC_DIR_ALL, BM_SETCHECK, BST_CHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_TYPE_ALL, BM_SETCHECK, BST_CHECKED, 0);
			return TRUE;
		}
		case WM_COMMAND:
		{
			HWND hWndMain;
			hWndMain = WindowList_Find(hWindowList,DlgParam->hContact);
			
			if ( wParam == IDCLOSE || wParam == IDCANCEL )
			{
				HMENU listMenu = GetMenu(hWndMain);
				EnableMenuItem(listMenu, 101, MF_BYCOMMAND | MF_ENABLED);
				DestroyWindow(hDlg);
				return TRUE;
			}
			else if ( wParam == IDSEARCH )
			{
				BYTE flags = 0x00;
				TCHAR filter[FILTERTEXT];
				LPTSTR buffer;
				int length;

				hListDlg = WindowList_Find(hWindowList, DlgParam->hContact);
				if ( hListDlg )
				{
					SetDlgItemText(hListDlg, IDC_MAIN, _T(""));
				
					if ( SendDlgItemMessage(hDlg, IDC_TYPE_WEB, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
						flags = flags | WLL_MAIL;
	
					if ( SendDlgItemMessage(hDlg, IDC_TYPE_MAIL, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
						flags = flags | WLL_URL;
	
					if ( SendDlgItemMessage(hDlg, IDC_DIR_IN, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
						flags = flags | WLL_OUT;
	
					if ( SendDlgItemMessage(hDlg, IDC_DIR_OUT, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
						flags = flags | WLL_IN;					
			
					if ( SendDlgItemMessage(hDlg, IDC_WHOLE_MESSAGE, BM_GETCHECK, 0, 0) == BST_CHECKED )
						flags = flags | SLL_DEEP;

					length = GetWindowTextLength(GetDlgItem(hDlg, IDC_SEARCHSTRING))+1;
					buffer = (LPTSTR)malloc( length*sizeof(TCHAR));
					GetDlgItemText(hDlg, IDC_SEARCHSTRING, buffer, length);
					WriteLinkList(hListDlg, flags, DlgParam->listStart, buffer, 0);
					free(buffer);
					
					mir_sntprintf(filter, _countof(filter), _T("%s: %s"), TXT_FILTER, TXT_SEARCHFILTER);
					SetDlgItemText(hWndMain, IDC_STATUS, filter);
				}
				break;
			}
		}
	}
	return FALSE;
}


/*
This function handles the options dialog messages
*/
INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BYTE useDefault;
	int mCol;

	switch(message)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hDlg);
			useDefault = db_get_b(NULL, LINKLIST_MODULE, LINKLIST_USE_DEF, 0xFF);
			if ( useDefault == 0x01 )
			{
				mCol = GetMirandaColour(&colourSet);
				if(mCol == 0)
				{
					SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg, IDC_INCOMING), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_BACKGROUND), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_IN), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_OUT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_BG), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_TXT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTIN), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTOUT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTBG), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTTXT), FALSE);
				}
				else
					useDefault = 0x00;

			}
			if (useDefault == 0x00 )
			{
				GetDBColour(&colourSet);
				SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_INCOMING, CPM_SETCOLOUR, 0, colourSet.incoming);
				SendDlgItemMessage(hDlg, IDC_OUTGOING, CPM_SETCOLOUR, 0, colourSet.outgoing);
				SendDlgItemMessage(hDlg, IDC_BACKGROUND, CPM_SETCOLOUR, 0, colourSet.background);
				SendDlgItemMessage(hDlg, IDC_TXT, CPM_SETCOLOUR, 0, colourSet.text);
			}
			
			//Shifted to the end of this function
			//WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text);

			// Init variables with current values from DB
			GetListOptions(&options);

			if(options.openNewWindow == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.openNewWindow == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK2, BM_SETCHECK, BST_CHECKED, 0);

			if(options.updateWindow == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.updateWindow == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK3, BM_SETCHECK, BST_CHECKED, 0);

			if(options.mouseEvent == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.mouseEvent == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK4, BM_SETCHECK, BST_CHECKED, 0);

			if(options.saveSpecial == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.saveSpecial == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK5, BM_SETCHECK, BST_CHECKED, 0);

			if(options.showDate == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.showDate == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK6, BM_SETCHECK, BST_CHECKED, 0);

			if(options.showLine == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK7, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.showLine == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK7, BM_SETCHECK, BST_CHECKED, 0);

			if(options.showTime == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK8, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.showTime == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK8, BM_SETCHECK, BST_CHECKED, 0);

			if(options.showDirection == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK9, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.showDirection == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK9, BM_SETCHECK, BST_CHECKED, 0);

			if(options.showType == 0x00)
				SendDlgItemMessage(hDlg, IDC_CHECK10, BM_SETCHECK, BST_UNCHECKED, 0);
			if(options.showType == 0x01)
				SendDlgItemMessage(hDlg, IDC_CHECK10, BM_SETCHECK, BST_CHECKED, 0);

			// Write example window
			WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
			
			return TRUE;
		}
		case WM_COMMAND:
		{
			if ( LOWORD(wParam) == IDC_DEFAULT_IN )
			{
				colourSet.incoming = IN_COL_DEF;
				SendDlgItemMessage(hDlg, IDC_INCOMING, CPM_SETCOLOUR, 0, colourSet.incoming);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				break;
			}
			if ( LOWORD(wParam) == IDC_DEFAULT_OUT )
			{
				colourSet.outgoing = OUT_COL_DEF;
				SendDlgItemMessage(hDlg, IDC_OUTGOING, CPM_SETCOLOUR, 0, colourSet.outgoing);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				break;
			}
			if ( LOWORD(wParam) == IDC_DEFAULT_BG )
			{
				colourSet.background = BG_COL_DEF;
				SendDlgItemMessage(hDlg, IDC_BACKGROUND, CPM_SETCOLOUR, 0, colourSet.background);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				break;
			}
			if ( LOWORD(wParam) == IDC_DEFAULT_TXT )
			{
				colourSet.text = TXT_COL_DEF;
				SendDlgItemMessage(hDlg, IDC_TXT, CPM_SETCOLOUR, 0, colourSet.text);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				break;
			}
			if ( LOWORD(wParam) == IDC_INCOMING )
			{
				colourSet.incoming = SendDlgItemMessage(hDlg, IDC_INCOMING, CPM_GETCOLOUR, 0, 0);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);				
				break;
			}
			if ( LOWORD(wParam) == IDC_OUTGOING )
			{
				colourSet.outgoing = SendDlgItemMessage(hDlg, IDC_OUTGOING, CPM_GETCOLOUR, 0, 0);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);				
				break;
			}
			if ( LOWORD(wParam) == IDC_BACKGROUND )
			{
				colourSet.background = SendDlgItemMessage(hDlg, IDC_BACKGROUND, CPM_GETCOLOUR, 0, 0);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);				
				break;
			}
			if ( LOWORD(wParam) == IDC_TXT )
			{
				colourSet.text = SendDlgItemMessage(hDlg, IDC_TXT, CPM_GETCOLOUR, 0, 0);
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);				
				break;
			}
			if ( LOWORD(wParam) == IDC_CHECK1 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if ( SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
				{
					EnableWindow(GetDlgItem(hDlg, IDC_INCOMING), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_BACKGROUND), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_IN), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_OUT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_BG), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_TXT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTIN), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTOUT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTBG), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_TXTTXT), TRUE);
					GetDBColour(&colourSet);
					SendDlgItemMessage(hDlg, IDC_INCOMING, CPM_SETCOLOUR, 0, colourSet.incoming);
					SendDlgItemMessage(hDlg, IDC_OUTGOING, CPM_SETCOLOUR, 0, colourSet.outgoing);
					SendDlgItemMessage(hDlg, IDC_BACKGROUND, CPM_SETCOLOUR, 0, colourSet.background);	
					SendDlgItemMessage(hDlg, IDC_TXT, CPM_SETCOLOUR, 0, colourSet.text);	
				}
				else
				{
					mCol = GetMirandaColour(&colourSet);
					if(mCol == 1)
					{
						MessageBox(NULL, TXT_NOSETTING, TXT_ERROR, MB_OK | MB_ICONEXCLAMATION);
						SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, BST_UNCHECKED, 0);
						break;
					}
					else
					{
						EnableWindow(GetDlgItem(hDlg, IDC_INCOMING), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BACKGROUND), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_TXT), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_IN), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_OUT), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_BG), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_TXT), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_TXTIN), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_TXTOUT), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_TXTBG), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_TXTTXT), FALSE);
					}
				}
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				break;		
			}
			
			if ( wParam == IDC_CHECK2 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if ( SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
					options.openNewWindow = 0;
				else
					options.openNewWindow = 1;
				
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);

				break;		
			}
			if ( wParam == IDC_CHECK3 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if ( SendDlgItemMessage(hDlg, IDC_CHECK3, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
					options.updateWindow = 0;
				else
					options.updateWindow = 1;
				
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				
				break;		
			}
			
			if ( wParam == IDC_CHECK4 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if ( SendDlgItemMessage(hDlg, IDC_CHECK4, BM_GETCHECK, 0, 0) == BST_UNCHECKED )
					options.mouseEvent = 0;
				else
					options.mouseEvent = 1;	

				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);

				break;		
			}

			if ( wParam == IDC_CHECK5 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if(SendDlgItemMessage(hDlg, IDC_CHECK5, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
					options.saveSpecial = 0;
				else
					options.saveSpecial = 1;
					
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);

				break;		
			}		

			if ( wParam == IDC_CHECK6 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if(SendDlgItemMessage(hDlg, IDC_CHECK6, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
					options.showDate = 0;
				else
					options.showDate = 1;
					
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);

				break;		
			}		

			if ( wParam == IDC_CHECK7 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if(SendDlgItemMessage(hDlg, IDC_CHECK7, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
					options.showLine = 0;
				else
					options.showLine = 1;	
				
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);

				break;		
			}		

			if ( wParam == IDC_CHECK8 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if(SendDlgItemMessage(hDlg, IDC_CHECK8, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
					options.showTime = 0;
				else
					options.showTime = 1;	
				
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				//db_set_b(NULL, LINKLIST_MODULE, LINKLIST_SHOW_TIME, 0x01);

				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				break;		
			}		

			if ( wParam == IDC_CHECK9 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if(SendDlgItemMessage(hDlg, IDC_CHECK9, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
					options.showDirection = 0;
				else
					options.showDirection = 1;	
				
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				//db_set_b(NULL, LINKLIST_MODULE, LINKLIST_SHOW_DIRECTION, 0x01);

				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				break;		
			}		

			if ( wParam == IDC_CHECK10 )
			{
				SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
				if(SendDlgItemMessage(hDlg, IDC_CHECK10, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
					options.showType = 0;
				else
					options.showType = 1;

				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				//db_set_b(NULL, LINKLIST_MODULE, LINKLIST_SHOW_TYPE, 0x01);
				
				WriteOptionExample(hDlg, colourSet.incoming, colourSet.outgoing, colourSet.background, colourSet.text, &options);
				break;		
			}		

		}

		case WM_NOTIFY:
		{
			if ( ((LPNMHDR)lParam)->code == PSN_APPLY )
			{
				// Write Settings to Database
				if ( SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0) == BST_CHECKED )
					db_set_b(NULL, LINKLIST_MODULE, LINKLIST_USE_DEF, 0x01);
				else
				{
					db_set_b(NULL, LINKLIST_MODULE, LINKLIST_USE_DEF, 0x00);
					colourSet.incoming = SendDlgItemMessage(hDlg, IDC_INCOMING, CPM_GETCOLOUR, 0, 0);
					colourSet.outgoing = SendDlgItemMessage(hDlg, IDC_OUTGOING, CPM_GETCOLOUR, 0, 0);
					colourSet.background = SendDlgItemMessage(hDlg, IDC_BACKGROUND, CPM_GETCOLOUR, 0, 0);
					colourSet.text = SendDlgItemMessage(hDlg, IDC_TXT, CPM_GETCOLOUR, 0, 0);
					SetDBColour(&colourSet);
				}

				SetListOptions(&options);

				// Write temporary values to Database




			}
		}
	}
	return FALSE;
}


/*
Progressbar
*/
LRESULT CALLBACK ProgressBarDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndBN_OK, hwndBN_Cancel, hwndEdit, hwndPB;
	static int cxChar, cyChar;

	switch ( message )
	{
		case WM_CREATE:
		{
			cxChar = LOWORD(GetDialogBaseUnits());
			cyChar = HIWORD(GetDialogBaseUnits());
			InitCommonControls();
			hwndPB = CreateWindowEx(0, PROGRESS_CLASS, _T(""), WS_CHILD | WS_VISIBLE, 0, 2, 343, 17, hwnd, NULL, hInst, NULL);
         	SendMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM (0, 100)); 
			return 0;
		}
		case WM_COMMAND:
		{
			if ( wParam == 100 )
				SendMessage(hwndPB, PBM_SETPOS, (WPARAM)lParam, 0);
			return 0;
		}
		case WM_DESTROY:
			return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}



/*
Splitter function.
Taken from srmm-plugin.... 
*/
LRESULT CALLBACK SplitterProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_NCHITTEST:
		return HTCLIENT;

	case WM_SETCURSOR:
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		SetCursor(splitCursor);
		return TRUE;
	}

	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		return 0;

	case WM_MOUSEMOVE:
		if ( GetCapture() == hWnd )
		{
			RECT rc;
			GetClientRect(hWnd, &rc);
			SendMessage( GetParent(hWnd), DM_LINKSPLITTER, (WPARAM)(HIWORD(GetMessagePos()) + rc.bottom / 2), (LPARAM)hWnd);
		}
		return 0;

	case WM_LBUTTONUP:
		ReleaseCapture();
		return 0;
	}
	return mir_callNextSubclass(hWnd, SplitterProc, msg, wParam, lParam);
}