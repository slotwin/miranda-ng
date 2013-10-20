#include "hdr/modern_commonheaders.h"
#include "hdr/modern_statusbar.h"
#include "./m_api/m_skin_eng.h"
#include "hdr/modern_commonprototypes.h"
#include "hdr/modern_clcpaint.h"
#include "hdr/modern_sync.h"

BOOL tooltipshoing;
POINT lastpnt;

#define TM_STATUSBAR 23435234
#define TM_STATUSBARHIDE 23435235

HWND hModernStatusBar = NULL;
HANDLE hFramehModernStatusBar = NULL;
extern void ApplyViewMode(const char *Name, bool onlySelector = false );
extern void SaveViewMode(const char *name, const TCHAR *szGroupFilter, const char *szProtoFilter, DWORD statusMask, DWORD stickyStatusMask, unsigned int options,  unsigned int stickies, unsigned int operators, unsigned int lmdat);

//int FindFrameID(HWND FrameHwnd);
COLORREF sttGetColor(char * module, char * color, COLORREF defColor);

#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

struct ProtoItemData : public MZeroedObject
{
	~ProtoItemData()
	{
	}

	HICON  icon;
	HICON  extraIcon;
	int    iconIndex;
	ptrA   ProtoName;
	ptrA   AccountName;
	int    ProtoStatus;
	ptrT   ProtoHumanName;
	ptrA   ProtoEMailCount;
	ptrT   ProtoStatusText;
	ptrT   ProtoXStatus;
	int    ProtoPos;
	int    fullWidth;
	RECT   protoRect;
	BOOL   DoubleIcons;

	BYTE   showProtoIcon;
	BYTE   showProtoName;
	BYTE   showStatusName;
	BYTE   xStatusMode;     // 0-only main, 1-xStatus, 2-main as overlay
	BYTE   connectingIcon;
	BYTE   showProtoEmails;
	BYTE   SBarRightClk;
	int    PaddingLeft;
	int    PaddingRight;

	bool   isDimmed;
};

static OBJLIST<ProtoItemData> ProtosData(5);

STATUSBARDATA g_StatusBarData = {0};

char* ApendSubSetting(char * buf, int size, char *first, char *second)
{
	mir_snprintf(buf, size, "%sFont%s", first, second);
	return buf;
}

int LoadStatusBarData()
{
	g_StatusBarData.perProtoConfig = db_get_b(NULL,"CLUI","SBarPerProto",SETTING_SBARPERPROTO_DEFAULT);
	g_StatusBarData.showProtoIcon = db_get_b(NULL,"CLUI","SBarShow",SETTING_SBARSHOW_DEFAULT)&1;
	g_StatusBarData.showProtoName = db_get_b(NULL,"CLUI","SBarShow",SETTING_SBARSHOW_DEFAULT)&2;
	g_StatusBarData.showStatusName = db_get_b(NULL,"CLUI","SBarShow",SETTING_SBARSHOW_DEFAULT)&4;
	g_StatusBarData.xStatusMode = (BYTE)( db_get_b(NULL,"CLUI","ShowXStatus",SETTING_SHOWXSTATUS_DEFAULT));
	g_StatusBarData.connectingIcon = db_get_b(NULL,"CLUI","UseConnectingIcon",SETTING_USECONNECTINGICON_DEFAULT);
	g_StatusBarData.showProtoEmails = db_get_b(NULL,"CLUI","ShowUnreadEmails",SETTING_SHOWUNREADEMAILS_DEFAULT);
	g_StatusBarData.SBarRightClk = db_get_b(NULL,"CLUI","SBarRightClk",SETTING_SBARRIGHTCLK_DEFAULT);

	g_StatusBarData.nProtosPerLine = db_get_b(NULL,"CLUI","StatusBarProtosPerLine",SETTING_PROTOSPERLINE_DEFAULT);
	g_StatusBarData.Align = db_get_b(NULL,"CLUI","Align",SETTING_ALIGN_DEFAULT);
	g_StatusBarData.VAlign = db_get_b(NULL,"CLUI","VAlign",SETTING_VALIGN_DEFAULT);
	g_StatusBarData.sameWidth = db_get_b(NULL,"CLUI","EqualSections",SETTING_EQUALSECTIONS_DEFAULT);
	g_StatusBarData.rectBorders.left = db_get_dw(NULL,"CLUI","LeftOffset",SETTING_LEFTOFFSET_DEFAULT);
	g_StatusBarData.rectBorders.right = db_get_dw(NULL,"CLUI","RightOffset",SETTING_RIGHTOFFSET_DEFAULT);
	g_StatusBarData.rectBorders.top = db_get_dw(NULL,"CLUI","TopOffset",SETTING_TOPOFFSET_DEFAULT);
	g_StatusBarData.rectBorders.bottom = db_get_dw(NULL,"CLUI","BottomOffset",SETTING_BOTTOMOFFSET_DEFAULT);
	g_StatusBarData.extraspace = (BYTE)db_get_dw(NULL,"CLUI","SpaceBetween",SETTING_SPACEBETWEEN_DEFAULT);

	if (g_StatusBarData.BarFont) DeleteObject(g_StatusBarData.BarFont);
	g_StatusBarData.BarFont = NULL;//LoadFontFromDB("ModernData","StatusBar",&g_StatusBarData.fontColor);
	
	int vis = db_get_b(NULL,"CLUI","ShowSBar",SETTING_SHOWSBAR_DEFAULT);
	int frameopt;
	int frameID = Sync( FindFrameID, hModernStatusBar );
	frameopt = CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,frameID),0);
	frameopt = frameopt & (~F_VISIBLE);
	if (vis) {
		ShowWindow(hModernStatusBar,SW_SHOW);
		frameopt |= F_VISIBLE;
	}
	else ShowWindow(hModernStatusBar,SW_HIDE);
	CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,frameID),frameopt);

	g_StatusBarData.TextEffectID = db_get_b(NULL,"StatusBar","TextEffectID",SETTING_TEXTEFFECTID_DEFAULT);
	g_StatusBarData.TextEffectColor1 = db_get_dw(NULL,"StatusBar","TextEffectColor1",SETTING_TEXTEFFECTCOLOR1_DEFAULT);
	g_StatusBarData.TextEffectColor2 = db_get_dw(NULL,"StatusBar","TextEffectColor2",SETTING_TEXTEFFECTCOLOR2_DEFAULT);

	if (g_StatusBarData.hBmpBackground) {DeleteObject(g_StatusBarData.hBmpBackground); g_StatusBarData.hBmpBackground = NULL;}

	if (g_CluiData.fDisableSkinEngine) {
		DBVARIANT dbv;
		g_StatusBarData.bkColour = sttGetColor("StatusBar","BkColour",CLCDEFAULT_BKCOLOUR);
		if ( db_get_b(NULL,"StatusBar","UseBitmap",CLCDEFAULT_USEBITMAP)) {
			if ( !db_get_s(NULL,"StatusBar","BkBitmap",&dbv)) {
				g_StatusBarData.hBmpBackground = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM)dbv.pszVal);
				db_free(&dbv);
			}
		}
		g_StatusBarData.bkUseWinColors = db_get_b(NULL,"StatusBar", "UseWinColours", CLCDEFAULT_USEWINDOWSCOLOURS);
		g_StatusBarData.backgroundBmpUse = db_get_w(NULL,"StatusBar","BkBmpUse",CLCDEFAULT_BKBMPUSE);
	}
	SendMessage(pcli->hwndContactList,WM_SIZE, 0, 0);

	return 1;
}

int BgStatusBarChange(WPARAM wParam,LPARAM lParam)
{
	if (MirandaExiting())
		return 0;
	
	LoadStatusBarData();
	return 0;
}

//ProtocolData;
int NewStatusPaintCallbackProc(HWND hWnd, HDC hDC, RECT *rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData)
{
	return ModernDrawStatusBar(hWnd,hDC);
}

int ModernDrawStatusBar(HWND hwnd, HDC hDC)
{
	if (hwnd == (HWND)-1) return 0;
	if (GetParent(hwnd) == pcli->hwndContactList)
		return ModernDrawStatusBarWorker(hwnd,hDC);
	
	CLUI__cliInvalidateRect(hwnd,NULL,FALSE);
	return 0;
}

int ModernDrawStatusBarWorker(HWND hWnd, HDC hDC)
{
	int iconHeight = GetSystemMetrics(SM_CYSMICON)+2;
	int i;

	// Count visible protos
	RECT rc;
	GetClientRect(hWnd, &rc);
	if (g_CluiData.fDisableSkinEngine) {
		if (g_StatusBarData.bkUseWinColors && xpt_IsThemed(g_StatusBarData.hTheme))
			xpt_DrawTheme(g_StatusBarData.hTheme, hWnd, hDC, 0, 0, &rc, &rc);           
		else
			DrawBackGround(hWnd, hDC,  g_StatusBarData.hBmpBackground, g_StatusBarData.bkColour, g_StatusBarData.backgroundBmpUse);
	}
	else SkinDrawGlyph(hDC, &rc, &rc, "Main,ID=StatusBar"); //TBD

	g_StatusBarData.nProtosPerLine = db_get_b(NULL,"CLUI","StatusBarProtosPerLine",SETTING_PROTOSPERLINE_DEFAULT);
	HFONT hOldFont = g_clcPainter.ChangeToFont(hDC,NULL,FONTID_STATUSBAR_PROTONAME,NULL);

	SIZE textSize = {0};
	GetTextExtentPoint32A(hDC, " ", 1, &textSize);
	int spaceWidth = textSize.cx;
	int textY = rc.top+((rc.bottom-rc.top-textSize.cy)>>1);
	int iconY = rc.top+((rc.bottom-rc.top-GetSystemMetrics(SM_CXSMICON))>>1);

	ProtosData.destroy();

	int protoCount;
	PROTOACCOUNT **accs;
	ProtoEnumAccounts( &protoCount, &accs );
	if (protoCount == 0)
		return 0;

	for (int j = 0; j < protoCount; j++) {
		int i = pcli->pfnGetAccountIndexByPos(j);
		if (i == -1 || !pcli->pfnGetProtocolVisibility(accs[i]->szModuleName))
			continue;
				
		char buf[256];
		mir_snprintf(buf, SIZEOF(buf), "SBarAccountIsCustom_%s", accs[i]->szModuleName);

		ProtoItemData *p = new ProtoItemData;

		if (g_StatusBarData.perProtoConfig && db_get_b(NULL, "CLUI", buf, SETTING_SBARACCOUNTISCUSTOM_DEFAULT)) {
			mir_snprintf(buf, SIZEOF(buf), "HideAccount_%s", accs[i]->szModuleName);
			if ( db_get_b(NULL, "CLUI", buf, SETTING_SBARHIDEACCOUNT_DEFAULT))
				continue;

			mir_snprintf(buf, SIZEOF(buf), "SBarShow_%s", accs[i]->szModuleName);

			BYTE showOps = db_get_b(NULL,"CLUI", buf, SETTING_SBARSHOW_DEFAULT);
			p->showProtoIcon = showOps & 1;
			p->showProtoName = showOps & 2;
			p->showStatusName = showOps & 4;

			mir_snprintf(buf, SIZEOF(buf), "ShowXStatus_%s", accs[i]->szModuleName);
			p->xStatusMode = db_get_b(NULL,"CLUI", buf, SETTING_SBARSHOW_DEFAULT);

			mir_snprintf(buf, SIZEOF(buf), "UseConnectingIcon_%s", accs[i]->szModuleName);
			p->connectingIcon = db_get_b(NULL,"CLUI", buf, SETTING_USECONNECTINGICON_DEFAULT);

			mir_snprintf(buf, SIZEOF(buf), "ShowUnreadEmails_%s", accs[i]->szModuleName);
			p->showProtoEmails = db_get_b(NULL,"CLUI", buf, SETTING_SHOWUNREADEMAILS_DEFAULT);

			mir_snprintf(buf, SIZEOF(buf), "SBarRightClk_%s", accs[i]->szModuleName);
			p->SBarRightClk = db_get_b(NULL,"CLUI", buf, SETTING_SBARRIGHTCLK_DEFAULT);

			mir_snprintf(buf, SIZEOF(buf), "PaddingLeft_%s", accs[i]->szModuleName);
			p->PaddingLeft = db_get_dw(NULL,"CLUI", buf, SETTING_PADDINGLEFT_DEFAULT);

			mir_snprintf(buf, SIZEOF(buf), "PaddingRight_%s", accs[i]->szModuleName);
			p->PaddingRight = db_get_dw(NULL,"CLUI", buf, SETTING_PADDINGRIGHT_DEFAULT);
		}
		else {
			p->showProtoIcon = g_StatusBarData.showProtoIcon;
			p->showProtoName = g_StatusBarData.showProtoName;
			p->showStatusName = g_StatusBarData.showStatusName;
			p->xStatusMode = g_StatusBarData.xStatusMode;
			p->connectingIcon = g_StatusBarData.connectingIcon;
			p->showProtoEmails = g_StatusBarData.showProtoEmails;
			p->SBarRightClk = 0;
			p->PaddingLeft = 0;
			p->PaddingRight = 0;
		}

		p->ProtoStatus = CallProtoService(accs[i]->szModuleName,PS_GETSTATUS, 0, 0);

		if (p->ProtoStatus > ID_STATUS_OFFLINE) {
			if (p->showProtoEmails == 1 && ProtoServiceExists(accs[i]->szModuleName, PS_GETUNREADEMAILCOUNT)) {
				char buf[40];
				mir_snprintf(buf, SIZEOF(buf),"[%d]", (int)ProtoCallService(accs[i]->szModuleName, PS_GETUNREADEMAILCOUNT, 0, 0));
				p->ProtoEMailCount = mir_strdup(buf);
			}
		}

		p->ProtoHumanName = mir_tstrdup(accs[i]->tszAccountName);
		p->AccountName = mir_strdup(accs[i]->szModuleName);
		p->ProtoName = mir_strdup(accs[i]->szProtoName);
		p->ProtoStatusText = mir_tstrdup(pcli->pfnGetStatusModeDescription(p->ProtoStatus, 0));
		p->ProtoPos = ProtosData.getCount();

		p->isDimmed = 0;
		if (g_CluiData.bFilterEffective & CLVM_FILTER_PROTOS) {
			char szTemp[2048];
			mir_snprintf(szTemp, SIZEOF(szTemp), "%s|", p->AccountName );
			p->isDimmed = strstr(g_CluiData.protoFilter, szTemp) ? 0 : 1;
		}

		ProtosData.insert(p);
	}

	if ( ProtosData.getCount() == 0)
		return 0;

	//START MULTILINE HERE 
	int orig_protoCount = protoCount;
	int orig_visProtoCount = ProtosData.getCount();
	int protosperline = 0;
	
	if (g_StatusBarData.nProtosPerLine)
		protosperline = g_StatusBarData.nProtosPerLine;
	else if (orig_visProtoCount)
		protosperline = orig_visProtoCount;
	else if (protoCount) {
		protosperline = protoCount;
		orig_visProtoCount = protoCount;
	}
	else {
		protosperline = 1;
		orig_visProtoCount = 1;
	}
	protosperline = min(protosperline,orig_visProtoCount);

	int linecount = protosperline ? (orig_visProtoCount+(protosperline-1))/protosperline : 1; //divide with rounding to up
	for (int line = 0; line < linecount; line++) {    
		int rowheight = max(textSize.cy+2, iconHeight);
		protoCount = min(protosperline,(orig_protoCount-line*protosperline));
		int visProtoCount = min(protosperline,(orig_visProtoCount-line*protosperline));
		GetClientRect(hWnd,&rc);

		rc.top += g_StatusBarData.rectBorders.top;
		rc.bottom -= g_StatusBarData.rectBorders.bottom;

		int aligndx = 0, maxwidth = 0, xstatus = 0, SumWidth = 0;

		int height = (rowheight*linecount);
		if (height > (rc.bottom - rc.top)) {
			rowheight = (rc.bottom - rc.top) / linecount;
			height = (rowheight*linecount);
		}

		int rowsdy = ((rc.bottom-rc.top)-height)/2;
		if (rowheight*(line)+rowsdy < rc.top-rowheight) continue;
		if (rowheight*(line+1)+rowsdy>rc.bottom+rowheight)
			break;

		if (g_StatusBarData.VAlign == 0) { //top
			rc.bottom = rc.top+rowheight*(line+1);
			rc.top = rc.top+rowheight*line+1;
		}
		else if (g_StatusBarData.VAlign == 1) { //center
			rc.bottom = rc.top+rowsdy+rowheight*(line+1);
			rc.top = rc.top+rowsdy+rowheight*line+1;
		}
		else if (g_StatusBarData.VAlign == 2) { //bottom
			rc.top = rc.bottom - (rowheight*(linecount - line));
			rc.bottom = rc.bottom - (rowheight*(linecount - line - 1)+1);
		}

		textY = rc.top + (((rc.bottom-rc.top) - textSize.cy)/2);
		iconY = rc.top + (((rc.bottom-rc.top) - iconHeight)/2);

		//Code for each line
		DWORD sw;
		int rectwidth = rc.right - rc.left - g_StatusBarData.rectBorders.left - g_StatusBarData.rectBorders.right;
		if (visProtoCount > 1)
			sw = (rectwidth - (g_StatusBarData.extraspace*(visProtoCount-1))) / visProtoCount;
		else
			sw = rectwidth;
		
		int *ProtoWidth = (int*)mir_alloc(sizeof(int)*visProtoCount);
		for (i=0; i < visProtoCount; i++) {
			ProtoItemData &p = ProtosData[line*protosperline + i];

			DWORD w = p.PaddingLeft;
			w += p.PaddingRight;

			if (p.showProtoIcon) {
				w += GetSystemMetrics(SM_CXSMICON)+1;

				p.extraIcon = NULL;
				if ((p.xStatusMode & 8) && p.ProtoStatus > ID_STATUS_OFFLINE) {
					TCHAR str[512];
					CUSTOM_STATUS cs = { sizeof(cs) };
					cs.flags = CSSF_MASK_NAME | CSSF_TCHAR;
					cs.ptszName = str;
					if ( CallProtoService(p.AccountName, PS_GETCUSTOMSTATUSEX, 0, (LPARAM)&cs) == 0)
						p.ProtoXStatus = mir_tstrdup(str);
				}
				
				if ((p.xStatusMode & 3)) {
					if (p.ProtoStatus > ID_STATUS_OFFLINE) {
						if ( ProtoServiceExists(p.AccountName, PS_GETCUSTOMSTATUSICON))
							p.extraIcon = (HICON)ProtoCallService(p.AccountName, PS_GETCUSTOMSTATUSICON, 0, 0);
						if (p.extraIcon && (p.xStatusMode & 3) == 3)
							w += GetSystemMetrics(SM_CXSMICON)+1;
					}
				}
			}

			SIZE textSize;
			if (p.showProtoName) {
				GetTextExtentPoint32(hDC, p.ProtoHumanName, lstrlen(p.ProtoHumanName), &textSize);
				w += textSize.cx + 3 + spaceWidth;
			}

			if (p.showProtoEmails && p.ProtoEMailCount) {
				GetTextExtentPoint32A(hDC, p.ProtoEMailCount, lstrlenA(p.ProtoEMailCount), &textSize);
				w += textSize.cx + 3 + spaceWidth;
			}

			if (p.showStatusName) {
				GetTextExtentPoint32(hDC, p.ProtoStatusText, lstrlen(p.ProtoStatusText), &textSize);
				w += textSize.cx + 3 + spaceWidth;
			}

			if ((p.xStatusMode & 8) && p.ProtoXStatus) {
				GetTextExtentPoint32(hDC, p.ProtoXStatus, lstrlen(p.ProtoXStatus), &textSize);
				w += textSize.cx + 3 + spaceWidth;
			}

			if (p.showProtoName || (p.showProtoEmails && p.ProtoEMailCount) || p.showStatusName || ((p.xStatusMode & 8) && p.ProtoXStatus))
				w -= spaceWidth;

			p.fullWidth = w;
			if (g_StatusBarData.sameWidth) {
				ProtoWidth[i] = sw;
				SumWidth += w;
			}
			else {
				ProtoWidth[i] = w;
				SumWidth += w;
			}
		}

		// Reposition rects
		for (i=0; i < visProtoCount; i++)
			if (ProtoWidth[i] > maxwidth)
				maxwidth = ProtoWidth[i];

		if (g_StatusBarData.sameWidth) {
			for (i=0; i < visProtoCount; i++)
				ProtoWidth[i] = maxwidth;
			SumWidth = maxwidth * visProtoCount;
		}
		SumWidth += (visProtoCount-1) * (g_StatusBarData.extraspace+1);

		if (SumWidth > rectwidth) {
			float f = (float)rectwidth/SumWidth;
			SumWidth = 0;
			for (i=0; i < visProtoCount; i++) {
				ProtoWidth[i] = (int)((float)ProtoWidth[i]*f);
				SumWidth += ProtoWidth[i];
			}
			SumWidth += (visProtoCount-1)*(g_StatusBarData.extraspace+1);
		}

		if (g_StatusBarData.Align == 1) //center
			aligndx = (rectwidth-SumWidth)>>1;
		else if (g_StatusBarData.Align == 2) //right
			aligndx = (rectwidth-SumWidth);

		// Draw in rects
		RECT r = rc;
		r.left += g_StatusBarData.rectBorders.left+aligndx;
		for (i=0; i < visProtoCount; i++) {
			ProtoItemData& p = ProtosData[line*protosperline + i];
			HRGN rgn;
			HICON hIcon = NULL;
			HICON hxIcon = NULL;
			BOOL NeedDestroy = FALSE;
			int x = r.left;
			x += p.PaddingLeft;
			r.right = r.left+ProtoWidth[i];

			if (p.showProtoIcon) {
				if (p.ProtoStatus > ID_STATUS_OFFLINE && (p.xStatusMode & 3) > 0) {
					if ( ProtoServiceExists(p.AccountName, PS_GETCUSTOMSTATUSICON)) {
						hxIcon = p.extraIcon;
						if (hxIcon) {
							if ((p.xStatusMode & 3) == 2) {
								hIcon = GetMainStatusOverlay(p.ProtoStatus);
								NeedDestroy = TRUE;
							}
							else if ((p.xStatusMode & 3) == 1) {
								hIcon = hxIcon;
								NeedDestroy = TRUE;
								hxIcon = NULL;
							}
						}
					}
				}

				if (hIcon == NULL && (hxIcon == NULL || ((p.xStatusMode & 3) == 3))) {
					if ((p.connectingIcon == 1) && p.ProtoStatus >= ID_STATUS_CONNECTING && p.ProtoStatus <= ID_STATUS_CONNECTING + MAX_CONNECT_RETRIES) {
						hIcon = (HICON)CLUI_GetConnectingIconService((WPARAM)p.AccountName,0);
						if (hIcon)
							NeedDestroy = TRUE;
						else
							hIcon = LoadSkinnedProtoIcon(p.AccountName,p.ProtoStatus);
					}
					else hIcon = LoadSkinnedProtoIcon(p.AccountName,p.ProtoStatus);
				}

				rgn = CreateRectRgn(r.left,r.top,r.right,r.bottom);

				if (g_StatusBarData.sameWidth) {
					int fw = p.fullWidth;
					int rw = r.right-r.left;
					if (g_StatusBarData.Align == 1)
						x = r.left+((rw-fw)/2);
					else if (g_StatusBarData.Align == 2)
						x = r.left+((rw-fw));
					else 
						x = r.left;
				}

				SelectClipRgn(hDC,rgn);
				p.DoubleIcons = FALSE;

				DWORD dim = p.isDimmed ? (( 64 << 24 ) | 0x80 ) : 0;

				if ((p.xStatusMode&3) == 3) {
					if (hIcon)
						mod_DrawIconEx_helper(hDC,x,iconY,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), 0, NULL,  DI_NORMAL|dim );
					if (hxIcon) {
						mod_DrawIconEx_helper(hDC,x+GetSystemMetrics(SM_CXSMICON)+1,iconY,hxIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), 0, NULL,DI_NORMAL|dim);
						x += GetSystemMetrics(SM_CXSMICON)+1;
					}
					p.DoubleIcons = hIcon && hxIcon;
				}
				else {
					if (hxIcon)
						mod_DrawIconEx_helper(hDC,x,iconY,hxIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), 0, NULL,DI_NORMAL|dim);
					if (hIcon)
						mod_DrawIconEx_helper(hDC,x,iconY,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), 0, NULL,DI_NORMAL| ((hxIcon && (p.xStatusMode&4))?(192 << 24):0 ) | dim );
				}

				if (hxIcon || hIcon) { /* TODO g_StatusBarData.bDrawLockOverlay  options to draw locked proto*/
					if ( db_get_b(NULL, p.AccountName,"LockMainStatus", 0)) {
						HICON hLockOverlay = LoadSkinnedIcon(SKINICON_OTHER_STATUS_LOCKED);
						if (hLockOverlay != NULL) {
							mod_DrawIconEx_helper(hDC, x, iconY, hLockOverlay, GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), 0, NULL,DI_NORMAL | dim);
							Skin_ReleaseIcon(hLockOverlay);
						}
					}
				}
				if (hxIcon) DestroyIcon_protect(hxIcon);
				if (NeedDestroy) DestroyIcon_protect(hIcon);
				else Skin_ReleaseIcon(hIcon);
				x += GetSystemMetrics(SM_CXSMICON)+1;
			}

			if (p.showProtoName) {
				SIZE textSize;
				RECT rt = r;
				rt.left = x+(spaceWidth>>1);
				rt.top = textY;
				ske_DrawText(hDC, p.ProtoHumanName, lstrlen(p.ProtoHumanName), &rt, 0);

				if ((p.showProtoEmails && p.ProtoEMailCount != NULL) || p.showStatusName || ((p.xStatusMode & 8) && p.ProtoXStatus)) {
					GetTextExtentPoint32(hDC, p.ProtoHumanName, lstrlen(p.ProtoHumanName), &textSize);
					x += textSize.cx + 3;
				}
			}

			if (p.showProtoEmails && p.ProtoEMailCount != NULL) {
				SIZE textSize;
				RECT rt = r;
				rt.left = x+(spaceWidth>>1);
				rt.top = textY;
				ske_DrawTextA(hDC, p.ProtoEMailCount, lstrlenA(p.ProtoEMailCount), &rt, 0);
				if (p.showStatusName || ((p.xStatusMode & 8) && p.ProtoXStatus)) {
					GetTextExtentPoint32A(hDC,p.ProtoEMailCount,lstrlenA(p.ProtoEMailCount),&textSize);
					x += textSize.cx+3;
				}
			}

			if (p.showStatusName) {
				SIZE textSize;
				RECT rt = r;
				rt.left = x+(spaceWidth>>1);
				rt.top = textY;
				ske_DrawText(hDC, p.ProtoStatusText, lstrlen(p.ProtoStatusText), &rt, 0);
				if (((p.xStatusMode & 8) && p.ProtoXStatus)) {
					GetTextExtentPoint32(hDC, p.ProtoStatusText, lstrlen(p.ProtoStatusText), &textSize);
					x += textSize.cx+3;
				}
			}

			if ((p.xStatusMode&8) && p.ProtoXStatus) {
				RECT rt = r;
				rt.left = x+(spaceWidth>>1);
				rt.top = textY;
				ske_DrawText(hDC, p.ProtoXStatus, lstrlen(p.ProtoXStatus), &rt, 0);
			}

			p.protoRect = r;

			r.left = r.right+g_StatusBarData.extraspace;
			DeleteObject(rgn);
		}
		mir_free(ProtoWidth);
	}

	SelectObject(hDC,hOldFont);
	ske_ResetTextEffect(hDC);
	return 0;
}

static BOOL _ModernStatus_OnExtraIconClick(int protoIndex)
{
	ProtoItemData &p = ProtosData[protoIndex];
	if ( !mir_strcmpi(p.ProtoName, "ICQ")) {
		if (p.ProtoStatus < ID_STATUS_ONLINE)
			return FALSE;

		HMENU hMainStatusMenu = (HMENU)CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
		if ( !hMainStatusMenu)
			return FALSE;

		HMENU hProtoStatusMenu = GetSubMenu( hMainStatusMenu, protoIndex );
		if ( !hProtoStatusMenu)
			return FALSE;

		int extraStatusMenuIndex = 1;
		HMENU hExtraStatusMenu = GetSubMenu( hProtoStatusMenu, extraStatusMenuIndex );
		if ( !hExtraStatusMenu)
			return FALSE;

		POINT pt; GetCursorPos( &pt );
		HWND hWnd = (HWND) CallService(MS_CLUI_GETHWND, 0 ,0 );
		TrackPopupMenu(hExtraStatusMenu, TPM_TOPALIGN|TPM_LEFTALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
		return TRUE;
	} 
	
	if ( !mir_strcmpi(p.ProtoName, "JABBER")) {
		if (p.ProtoStatus < ID_STATUS_ONLINE)
			return FALSE;

		// Show Moods
		#define PS_JABBER_MOOD "/AdvStatusSet/Mood"
		if ( ProtoServiceExists(p.AccountName, PS_JABBER_MOOD)) {
			ProtoCallService(p.AccountName, PS_JABBER_MOOD, 0, 0);
			return TRUE;
		}
	}
	return FALSE;
}

#define TOOLTIP_TOLERANCE 5
LRESULT CALLBACK ModernStatusProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    static POINT ptToolTipShow = {0};
    switch (msg)  {
    case WM_CREATE:
		g_StatusBarData.hTheme = xpt_AddThemeHandle(hwnd, L"STATUS");
		break;

	case WM_DESTROY:
		xpt_FreeThemeForWindow(hwnd);
		ProtosData.destroy();
		break;

	case WM_SIZE:
		if ( !g_CluiData.fLayered || GetParent(hwnd) != pcli->hwndContactList)
			InvalidateRect(hwnd, NULL, FALSE);
		return DefWindowProc(hwnd, msg, wParam, lParam);

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		if (GetParent(hwnd) == pcli->hwndContactList && g_CluiData.fLayered)
			CallService(MS_SKINENG_INVALIDATEFRAMEIMAGE,(WPARAM)hwnd,0);
		else if (GetParent(hwnd) == pcli->hwndContactList && !g_CluiData.fLayered) {
			RECT rc = {0};
			GetClientRect(hwnd, &rc);
			rc.right++;
			rc.bottom++;
			HDC hdc = GetDC(hwnd);
			HDC hdc2 = CreateCompatibleDC(hdc);
			HBITMAP hbmp = ske_CreateDIB32(rc.right, rc.bottom);
			HBITMAP hbmpo = (HBITMAP)SelectObject(hdc2, hbmp);  
			SetBkMode(hdc2,TRANSPARENT);
			ske_BltBackImage(hwnd, hdc2, &rc);
			ModernDrawStatusBarWorker(hwnd,hdc2);
			BitBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, hdc2, rc.left, rc.top, SRCCOPY);
			SelectObject(hdc2, hbmpo);
			DeleteObject(hbmp);
			DeleteDC(hdc2);

			SelectObject(hdc,GetStockObject(DEFAULT_GUI_FONT));
			ReleaseDC(hwnd,hdc);
			ValidateRect(hwnd,NULL);
		}
		else {
			RECT rc;
			GetClientRect(hwnd,&rc);

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd,&ps);
			HDC hdc2 = CreateCompatibleDC(hdc);
			HBITMAP hbmp = ske_CreateDIB32(rc.right,rc.bottom);
			HBITMAP hbmpo = (HBITMAP) SelectObject(hdc2,hbmp);

			HBRUSH br = GetSysColorBrush(COLOR_3DFACE);
			FillRect(hdc2,&ps.rcPaint,br);
			ModernDrawStatusBarWorker(hwnd,hdc2);
			BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,
				hdc2,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
			SelectObject(hdc2,hbmpo);
			DeleteObject(hbmp);
			DeleteDC(hdc2);
			ps.fErase = FALSE;
			EndPaint(hwnd,&ps);
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);

	case WM_GETMINMAXINFO:
		{
			RECT rct;
			GetWindowRect(hwnd,&rct);
			memset((LPMINMAXINFO)lParam, 0, sizeof(MINMAXINFO));
			((LPMINMAXINFO)lParam)->ptMinTrackSize.x = 16;
			((LPMINMAXINFO)lParam)->ptMinTrackSize.y = 16;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.x = 1600;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.y = 1600;
		}
		return 0;

	case WM_SHOWWINDOW:
		{
			if (tooltipshoing) {
				NotifyEventHooks(g_CluiData.hEventStatusBarHideToolTip, 0, 0);
				tooltipshoing = FALSE;
			}
			int ID = Sync(FindFrameID, hwnd);
			if (ID) {
				int res = CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS,ID),0);
				if (res >= 0)
					db_set_b(0, "CLUI","ShowSBar",(BYTE)(wParam/*(res&F_VISIBLE)*/?1:0));
			}
		}
		break;

	case WM_TIMER:
		if (wParam == TM_STATUSBARHIDE) {
			KillTimer(hwnd,TM_STATUSBARHIDE);
			if (tooltipshoing) {
				NotifyEventHooks(g_CluiData.hEventStatusBarHideToolTip, 0, 0);
				tooltipshoing = FALSE;
				ReleaseCapture();
			}
		}
		else if (wParam == TM_STATUSBAR) {
			POINT pt;
			KillTimer(hwnd,TM_STATUSBAR);
			GetCursorPos(&pt);
			if (pt.x == lastpnt.x && pt.y == lastpnt.y) {
				RECT rc;
				ScreenToClient(hwnd, &pt);
				for (int i=0; i < ProtosData.getCount(); i++) {
					rc = ProtosData[i].protoRect;
					if (PtInRect(&rc,pt)) {
						NotifyEventHooks(g_CluiData.hEventStatusBarShowToolTip,(WPARAM)ProtosData[i].AccountName,0);
						CLUI_SafeSetTimer(hwnd,TM_STATUSBARHIDE,db_get_w(NULL,"CLUIFrames","HideToolTipTime",SETTING_HIDETOOLTIPTIME_DEFAULT),0);
						tooltipshoing = TRUE;
						ClientToScreen(hwnd,&pt);
						ptToolTipShow = pt;
						SetCapture(hwnd);
						return 0;
					}
				}
				return 0;
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		if (tooltipshoing) {
			POINT pt;
			GetCursorPos(&pt);
			if ( abs(pt.x-ptToolTipShow.x) > TOOLTIP_TOLERANCE || abs(pt.y-ptToolTipShow.y) > TOOLTIP_TOLERANCE) {
				KillTimer(hwnd,TM_STATUSBARHIDE);
				NotifyEventHooks(g_CluiData.hEventStatusBarHideToolTip, 0, 0);
				tooltipshoing = FALSE;
				ReleaseCapture();
			}
		}
		break;

	case WM_SETCURSOR:
		if (g_CluiData.bBehindEdgeSettings) CLUI_UpdateTimer(0); 
		{
			POINT pt;
			GetCursorPos(&pt);
			SendMessage(GetParent(hwnd),msg,wParam,lParam);
			if (pt.x == lastpnt.x && pt.y == lastpnt.y)
				return(CLUI_TestCursorOnBorders());

			lastpnt = pt;
			if (tooltipshoing)
				if  ( abs(pt.x-ptToolTipShow.x) > TOOLTIP_TOLERANCE || abs(pt.y-ptToolTipShow.y) > TOOLTIP_TOLERANCE) {
					KillTimer(hwnd,TM_STATUSBARHIDE);
					NotifyEventHooks(g_CluiData.hEventStatusBarHideToolTip, 0, 0);
					tooltipshoing = FALSE;
					ReleaseCapture();
				}
			KillTimer(hwnd,TM_STATUSBAR);
			CLUI_SafeSetTimer(hwnd, TM_STATUSBAR, db_get_w(NULL,"CLC","InfoTipHoverTime", CLCDEFAULT_INFOTIPTIME),0);
		}
		return CLUI_TestCursorOnBorders();

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			RECT rc;
			POINT pt = UNPACK_POINT(lParam);
			KillTimer(hwnd,TM_STATUSBARHIDE);
			KillTimer(hwnd,TM_STATUSBAR);

			if (tooltipshoing)
				NotifyEventHooks(g_CluiData.hEventStatusBarHideToolTip, 0, 0);

			tooltipshoing = FALSE;
			for (int i=0; i < ProtosData.getCount(); i++) {
				ProtoItemData& p = ProtosData[i];
				BOOL isOnExtra = FALSE;

				rc = p.protoRect;
				RECT rc1 = rc;
				rc1.left = rc.left+16;
				rc1.right = rc1.left+16;
				if ( PtInRect(&rc, pt) && PtInRect(&rc1,pt) && p.DoubleIcons)
					isOnExtra = TRUE;

				if ( PtInRect(&rc,pt)) {
					HMENU hMenu = NULL;

					BOOL bShift  = ( GetKeyState( VK_SHIFT ) & 0x8000);
					BOOL bCtrl   = ( GetKeyState( VK_CONTROL ) & 0x8000);

					if ((msg == WM_MBUTTONDOWN || (msg == WM_RBUTTONDOWN && bCtrl) || isOnExtra) && _ModernStatus_OnExtraIconClick( i ))
						return TRUE;

					if (msg == WM_LBUTTONDOWN && bCtrl) {
						if ( g_CluiData.bFilterEffective != CLVM_FILTER_PROTOS || !bShift ) {
							ApplyViewMode("");
							mir_snprintf( g_CluiData.protoFilter, SIZEOF(g_CluiData.protoFilter), "%s|", p.AccountName );
							g_CluiData.bFilterEffective = CLVM_FILTER_PROTOS;
						}
						else {
							char protoF[ sizeof(g_CluiData.protoFilter) ];
							mir_snprintf( protoF, SIZEOF(protoF), "%s|", p.AccountName );
							char *pos = strstri( g_CluiData.protoFilter, p.AccountName );
							if (pos) {
								// remove filter
								int len = strlen( protoF );
								memmove( pos, pos + len, strlen( pos + len ) + 1 );

								if ( strlen( g_CluiData.protoFilter ) == 0 )
									ApplyViewMode("");
								else
									g_CluiData.bFilterEffective = CLVM_FILTER_PROTOS;
							}
							else {
								//add filter
								mir_snprintf( g_CluiData.protoFilter, SIZEOF(g_CluiData.protoFilter), "%s%s", g_CluiData.protoFilter, protoF );
								g_CluiData.bFilterEffective = CLVM_FILTER_PROTOS;
							}
						}

						if (g_CluiData.bFilterEffective == CLVM_FILTER_PROTOS) {
							char filterName[ sizeof(g_CluiData.protoFilter) ] = { 0 };
							filterName[0] = (char)13;

							int protoCount;
							PROTOACCOUNT ** accs;
							ProtoEnumAccounts( &protoCount, &accs );

							bool first = true;
							for (int pos = 0; pos < protoCount; pos++) {
								int i = pcli->pfnGetAccountIndexByPos( pos );
								if ( i < 0 && i >= protoCount )
									continue;

								char protoF[ sizeof(g_CluiData.protoFilter) ];
								mir_snprintf( protoF, SIZEOF(protoF), "%s|", accs[i]->szModuleName );
								if ( strstri(g_CluiData.protoFilter, protoF)) {
									char * temp = mir_utf8encodeT( accs[i]->tszAccountName );
									if ( !first )
										strncat( filterName, "; ", SIZEOF(filterName) - strlen(filterName));
									strncat( filterName, temp, SIZEOF(filterName) - strlen(filterName));
									first = false;
									mir_free( temp );
								}
							}

							SaveViewMode( filterName, _T(""), g_CluiData.protoFilter, 0, -1, 0, 0, 0, 0 );

							ApplyViewMode( filterName );
						}
						pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
						CLUI__cliInvalidateRect( hwnd, NULL, FALSE );
						SetCapture( NULL );
						return 0;
					}

					if ( !hMenu) {
						if (msg == WM_RBUTTONDOWN) {
							BOOL a = ((g_StatusBarData.perProtoConfig && p.SBarRightClk) || g_StatusBarData.SBarRightClk );
							if ( a ^ bShift )
								hMenu = (HMENU)CallService(MS_CLIST_MENUGETMAIN, 0, 0);
							else
								hMenu = (HMENU)CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
						}
						else {
							hMenu = (HMENU)CallService(MS_CLIST_MENUGETSTATUS, 0, 0);  
							unsigned int cpnl = 0;
							int mcnt = GetMenuItemCount(hMenu);
							for (int j = 0; j < mcnt; ++j) {
								HMENU hMenus = GetSubMenu(hMenu, j);
								if (hMenus && cpnl++ == i) { 
									hMenu = hMenus; 
									break; 
								}
							}
						}
					}

					ClientToScreen(hwnd,&pt);

					HWND parent = GetParent(hwnd);
					if (parent != pcli->hwndContactList) parent = GetParent(parent);
					TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y, 0, parent,NULL);
					return 0;
				}
			}

			GetClientRect(hwnd, &rc);
			if ( PtInRect( &rc, pt ) && msg == WM_LBUTTONDOWN && g_CluiData.bFilterEffective == CLVM_FILTER_PROTOS) {
				ApplyViewMode("");
				CLUI__cliInvalidateRect( hwnd, NULL, FALSE );
				SetCapture( NULL );
				return 0;
			}
			return SendMessage(GetParent(hwnd), msg, wParam, lParam);
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND StatusBar_Create(HWND parent)
{
	WNDCLASS wndclass = {0};
	TCHAR pluginname[] = _T("ModernStatusBar");
	int h = GetSystemMetrics(SM_CYSMICON)+2;
	if ( GetClassInfo(g_hInst, pluginname, &wndclass) == 0) {
		wndclass.lpfnWndProc = ModernStatusProc;
		wndclass.hInstance = g_hInst;
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		wndclass.lpszClassName = pluginname;
		RegisterClass(&wndclass);
	}

	hModernStatusBar = CreateWindow(pluginname, pluginname, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 0, h, parent, NULL, g_hInst, NULL);

	// register frame
	CLISTFrame Frame = { sizeof(Frame) };
	Frame.hWnd = hModernStatusBar;
	Frame.align = alBottom;
	Frame.hIcon = LoadSkinnedIcon (SKINICON_OTHER_FRAME);
	Frame.Flags = F_LOCKED | F_NOBORDER | F_NO_SUBCONTAINER | F_TCHAR;
	if ( db_get_b(NULL, "CLUI", "ShowSBar", SETTING_SHOWSBAR_DEFAULT))
		Frame.Flags |= F_VISIBLE;
	Frame.height = h;
	Frame.tname = _T("Status Bar");
	Frame.TBtname = TranslateT("Status Bar");
	hFramehModernStatusBar = (HANDLE)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,0);
	CallService(MS_SKINENG_REGISTERPAINTSUB,(WPARAM)Frame.hWnd,(LPARAM)NewStatusPaintCallbackProc); //$$$$$ register sub for frame

	LoadStatusBarData();
	cliCluiProtocolStatusChanged(0, 0);
	CallService(MS_CLIST_FRAMES_UPDATEFRAME,-1,0);
	return hModernStatusBar;
}
