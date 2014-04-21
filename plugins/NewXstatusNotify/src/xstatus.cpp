/*
	NewXstatusNotify YM - Plugin for Miranda IM
	Copyright (c) 2007-2011 yaho

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common.h"

XSTATUSCHANGE *NewXSC(MCONTACT hContact, char *szProto, int xstatusType, int action, TCHAR *stzTitle, TCHAR *stzText)
{
	XSTATUSCHANGE *xsc = (XSTATUSCHANGE *)mir_alloc(sizeof(XSTATUSCHANGE));
	xsc->hContact = hContact;
	xsc->szProto = szProto;
	xsc->type = xstatusType;
	xsc->action = action;
	xsc->stzTitle = stzTitle;
	xsc->stzText = stzText;
	return xsc;
}

void FreeXSC(XSTATUSCHANGE *xsc)
{
	if (xsc) {
		mir_free(xsc->stzTitle);
		mir_free(xsc->stzText);
		mir_free(xsc);
		xsc = NULL;
	}
}

void RemoveLoggedEvents(MCONTACT hContact)
{
	for (int i = eventList.getCount() - 1; i >= 0; i--) {
		DBEVENT *dbevent = eventList[i];
		if (dbevent->hContact == hContact) {
			db_event_delete(dbevent->hContact, dbevent->hDBEvent);
			eventList.remove(i);
			mir_free(dbevent);
		}
	}
}

TCHAR* GetStatusTypeAsString(int type, TCHAR *buff)
{
	switch (type) {
	case TYPE_JABBER_MOOD:
		_tcscpy(buff, TranslateT("Mood")); break;
	case TYPE_JABBER_ACTIVITY:
		_tcscpy(buff, TranslateT("Activity")); break;
	case TYPE_ICQ_XSTATUS:
		_tcscpy(buff, TranslateT("Xstatus")); break;
	default:
		_tcscpy(buff, TranslateT("<unknown>"));
	}

	return buff;
}

TCHAR* ReplaceVars(XSTATUSCHANGE *xsc , TCHAR *tmplt, TCHAR *str)
{
	TCHAR tmp[1024];

	if (tmplt == NULL || tmplt[0] == _T('\0'))
		return NULL;

	str[0] = _T('\0');
	int len = lstrlen(tmplt);

	for (int i = 0; i < len; i++) {
		tmp[0] = _T('\0');

		if (tmplt[i] == _T('%')) {
			i++;
			switch (tmplt[i]) {
			case 'n':
				{
					TCHAR stzType[32];
					lstrcpyn(tmp, GetStatusTypeAsString(xsc->type, stzType), SIZEOF(tmp));
				}
				break;

			case 't':
				if (xsc->stzTitle == NULL || xsc->stzTitle[0] == _T('\0'))
					lstrcpyn(tmp, TranslateT("<no title>"), SIZEOF(tmp));
				else
					lstrcpyn(tmp, xsc->stzTitle, SIZEOF(tmp));
				break;

			case 'm':
				if (xsc->stzText == NULL || xsc->stzText[0] == _T('\0'))
					lstrcpyn(tmp, TranslateT("<no status message>"), SIZEOF(tmp));
				else {
					TCHAR *_tmp = AddCR(xsc->stzText);
					lstrcpyn(tmp, _tmp, SIZEOF(tmp));
					mir_free(_tmp);
				}
				break;

			case 'c':
				if (xsc->hContact == NULL)
					lstrcpyn(tmp, TranslateT("Contact"), SIZEOF(tmp));
				else
					lstrcpyn(tmp, (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)xsc->hContact, GCDNF_TCHAR), SIZEOF(tmp));
				break;

			default:
				i--;
				tmp[0] = tmplt[i], tmp[1] = _T('\0');
				break;
			}
		}
		else if (tmplt[i] == _T('\\')) {
			i++;
			switch (tmplt[i]) {
			case 'n':
				tmp[0] = _T('\r'), tmp[1] = _T('\n'), tmp[2] = _T('\0');
				break;
			case 't':
				tmp[0] = _T('\t'), tmp[1] = _T('\0');
				break;
			default:
				i--;
				tmp[0] = tmplt[i], tmp[1] = _T('\0');
				break;
			}
		}
		else tmp[0] = tmplt[i], tmp[1] = _T('\0');

		if (tmp[0] != _T('\0')) {
			if (lstrlen(tmp) + lstrlen(str) < SIZEOF(str))
				lstrcat(str, tmp);
			else {
				lstrcat(str, _T("..."));
				break;
			}
		}
	}

	return str;
}

void ShowPopup(XSTATUSCHANGE *xsc)
{
	DBVARIANT dbv;
	char szSetting[64];

	POPUPDATAT ppd = {0};
	ppd.lchContact = xsc->hContact;

	switch (xsc->type) {
	case TYPE_JABBER_MOOD:
	case TYPE_JABBER_ACTIVITY:
		mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/%s", xsc->szProto, (xsc->type == TYPE_JABBER_MOOD) ? "mood" : "activity", "icon");
		if (!db_get_s(xsc->hContact, "AdvStatus", szSetting, &dbv)) {
			ppd.lchIcon = Skin_GetIcon(dbv.pszVal);
			db_free(&dbv);
		}
		break;
	case TYPE_ICQ_XSTATUS:
		{
			int statusId = db_get_b(xsc->hContact, xsc->szProto, "XStatusId", 0);
			ppd.lchIcon = (HICON)CallProtoService(xsc->szProto, PS_GETCUSTOMSTATUSICON, statusId, LR_SHARED);
		}
	}

	if (ppd.lchIcon == NULL)
		ppd.lchIcon = LoadSkinnedProtoIcon(xsc->szProto, db_get_w(xsc->hContact, xsc->szProto, "Status", ID_STATUS_ONLINE));

	switch (opt.Colors) {
	case POPUP_COLOR_OWN:
		ppd.colorBack = db_get_dw(0, MODULE, "40081bg", COLOR_BG_AVAILDEFAULT);
		ppd.colorText = db_get_dw(0, MODULE, "40081tx", COLOR_TX_DEFAULT);
		break;
	case POPUP_COLOR_WINDOWS:
		ppd.colorBack = GetSysColor(COLOR_BTNFACE);
		ppd.colorText = GetSysColor(COLOR_WINDOWTEXT);
		break;
	case POPUP_COLOR_POPUP:
		ppd.colorBack = ppd.colorText = 0;
		break;
	}

	_tcsncpy(ppd.lptzContactName, (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, xsc->hContact, GSMDF_TCHAR), MAX_CONTACTNAME);

	// add group name to popup title
	if (opt.ShowGroup) {
		DBVARIANT dbv;
		if (!db_get_ts(xsc->hContact, "CList", "Group", &dbv)) {
			_tcsncat(ppd.lptzContactName, _T(" ("), MAX_CONTACTNAME);
			_tcsncat(ppd.lptzContactName, dbv.ptszVal, MAX_CONTACTNAME);
			_tcsncat(ppd.lptzContactName, _T(")"), MAX_CONTACTNAME);
			db_free(&dbv);
		}
	}

	// cut message if needed
	if (opt.PTruncateMsg && (opt.PMsgLen > 0) && xsc->stzText && (_tcslen(xsc->stzText) > opt.PMsgLen)) {
		TCHAR buff[MAX_TEXT_LEN + 3];
		_tcsncpy(buff, xsc->stzText, opt.PMsgLen);
		buff[opt.PMsgLen] = 0;
		_tcscat(buff, _T("..."));
		mir_free(xsc->stzText);
		xsc->stzText = mir_tstrdup(buff);
	}

	TCHAR *Template = _T("");
	switch (xsc->action) {
	case NOTIFY_NEW_XSTATUS:
		Template = templates.PopupXstatusChanged; break;
	case NOTIFY_NEW_MESSAGE:
		Template = templates.PopupMsgChanged; break;
	case NOTIFY_REMOVE_XSTATUS:
		Template = templates.PopupXstatusRemoved; break;
	case NOTIFY_REMOVE_MESSAGE:
		Template = templates.PopupMsgRemoved; break;
	case NOTIFY_OPENING_ML:
		Template = templates.LogOpening; break;
	}

	TCHAR stzPopupText[2 * MAX_TEXT_LEN];
	ReplaceVars(xsc, Template, stzPopupText);
	_tcsncpy(ppd.lptzText, stzPopupText, SIZEOF(ppd.lptzText));
	ppd.lptzText[SIZEOF(ppd.lptzText) - 1] = 0;

	ppd.PluginWindowProc = PopupDlgProc;
	ppd.iSeconds = opt.PopupTimeout;
	PUAddPopupT(&ppd);
}

void PlayXStatusSound(int action)
{
	switch (action) {
	case NOTIFY_NEW_XSTATUS:
		SkinPlaySound(XSTATUS_SOUND_CHANGED); break;
	case NOTIFY_NEW_MESSAGE:
		SkinPlaySound(XSTATUS_SOUND_MSGCHANGED); break;
	case NOTIFY_REMOVE_XSTATUS:
		SkinPlaySound(XSTATUS_SOUND_REMOVED); break;
	case NOTIFY_REMOVE_MESSAGE:
		SkinPlaySound(XSTATUS_SOUND_MSGREMOVED); break;
	}
}

void LogToMessageWindow(XSTATUSCHANGE *xsc, BOOL opening)
{
	// cut message if needed
	if (opt.LTruncateMsg && (opt.LMsgLen > 0) && xsc->stzText && (_tcslen(xsc->stzText) > opt.LMsgLen)) {
		TCHAR buff[MAX_TEXT_LEN + 3];
		_tcsncpy(buff, xsc->stzText, opt.LMsgLen);
		buff[opt.LMsgLen] = 0;
		_tcscat(buff, _T("..."));
		mir_free(xsc->stzText);
		xsc->stzText = mir_tstrdup(buff);
	}

	TCHAR *Template = _T("");
	switch (xsc->action) {
	case NOTIFY_NEW_XSTATUS:
		Template = templates.LogNewXstatus; break;
	case NOTIFY_NEW_MESSAGE:
		Template = templates.LogNewMsg; break;
	case NOTIFY_REMOVE_XSTATUS:
		Template = templates.LogRemove; break;
	case NOTIFY_OPENING_ML:
		Template = templates.LogOpening; break;
	}

	TCHAR stzLogText[2 * MAX_TEXT_LEN], stzLastLog[2 * MAX_TEXT_LEN];
	ReplaceVars(xsc, Template, stzLogText);
	DBGetStringDefault(xsc->hContact, MODULE, DB_LASTLOG, stzLastLog, SIZEOF(stzLastLog), _T(""));

	if (!opt.KeepInHistory || !(opt.PreventIdentical && _tcscmp(stzLastLog, stzLogText) == 0)) {
		db_set_ws(xsc->hContact, MODULE, DB_LASTLOG, stzLogText);

		char *blob = mir_utf8encodeT(stzLogText);

		DBEVENTINFO dbei = {0};
		dbei.cbSize = sizeof(dbei);
		dbei.cbBlob = (DWORD)strlen(blob) + 1;
		dbei.pBlob = (PBYTE)blob;
		dbei.eventType = EVENTTYPE_STATUSCHANGE;
		dbei.flags = DBEF_READ | DBEF_UTF;

		dbei.timestamp = (DWORD)time(NULL);
		dbei.szModule = MODULE;
		HANDLE hDBEvent = db_event_add(xsc->hContact, &dbei);
		mir_free(blob);

		if (!opt.KeepInHistory) {
			DBEVENT *dbevent = (DBEVENT *)mir_alloc(sizeof(DBEVENT));
			dbevent->hContact = xsc->hContact;
			dbevent->hDBEvent = hDBEvent;
			eventList.insert(dbevent);
		}
	}
}

void LogChangeToFile(XSTATUSCHANGE *xsc)
{
	TCHAR stzType[32], stzDate[32], stzTime[32], stzText[MAX_TEXT_LEN];

	GetStatusTypeAsString(xsc->type, stzType);

	INT_PTR stzName = CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)xsc->hContact, GCDNF_TCHAR);

	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL,_T("HH':'mm"), stzTime, SIZEOF(stzTime));
	GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL,_T("dd/MM/yyyy"), stzDate, SIZEOF(stzDate));

	if (xsc->action == NOTIFY_REMOVE_XSTATUS)
		mir_sntprintf(stzText, SIZEOF(stzText), TranslateT("%s, %s. %s removed %s.\r\n"), stzDate, stzTime, stzName, stzType);
	else
		mir_sntprintf(stzText, SIZEOF(stzText), TranslateT("%s, %s. %s changed %s to: %s.\r\n"), stzDate, stzTime, stzName, stzType, xsc->stzTitle);

	LogToFile(stzText);
}

void ExtraStatusChanged(XSTATUSCHANGE *xsc)
{
	if (xsc == NULL)
		return;

	BOOL bEnablePopup = true, bEnableSound = true;
	char buff[12] = {0};

	mir_snprintf(buff, SIZEOF(buff), "%d", ID_STATUS_EXTRASTATUS);

	if ((db_get_b(0, MODULE, buff, 1) == 0)
		|| (db_get_w(xsc->hContact, xsc->szProto, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
		|| (!opt.HiddenContactsToo && db_get_b(xsc->hContact, "CList", "Hidden", 0))
		|| (opt.TempDisabled)
		|| (opt.IgnoreEmpty && (xsc->stzTitle == NULL || xsc->stzTitle[0] == '\0') && (xsc->stzText == NULL || xsc->stzText[0] == '\0')))
	{
		FreeXSC(xsc);
		return;
	}

	char statusIDs[12], statusIDp[12];
	if (opt.AutoDisable) {
		WORD myStatus = (WORD)CallProtoService(xsc->szProto, PS_GETSTATUS, 0, 0);
		mir_snprintf(statusIDs, SIZEOF(statusIDs), "s%d", myStatus);
		mir_snprintf(statusIDp, SIZEOF(statusIDp), "p%d", myStatus);
		bEnableSound = db_get_b(0, MODULE, statusIDs, 1) ? FALSE : TRUE;
		bEnablePopup = db_get_b(0, MODULE, statusIDp, 1) ? FALSE : TRUE;
	}

	if (!(templates.PopupFlags & xsc->action))
		bEnableSound = bEnablePopup = false;

	int xstatusID = db_get_b(xsc->hContact, xsc->szProto, "XStatusId", 0);
	if (opt.PDisableForMusic && xsc->type == TYPE_ICQ_XSTATUS && xstatusID == XSTATUS_MUSIC)
		bEnableSound = bEnablePopup = false;

	if (bEnablePopup && db_get_b(xsc->hContact, MODULE, "EnableXStatusNotify", 1) && db_get_b(0, MODULE, xsc->szProto, 1))
		ShowPopup(xsc);

	if (bEnableSound && db_get_b(xsc->hContact, MODULE, "EnableXStatusNotify", 1))
		PlayXStatusSound(xsc->action);

	BYTE enableLog = opt.EnableLogging;
	if (opt.LDisableForMusic && xsc->type == TYPE_ICQ_XSTATUS && xstatusID == XSTATUS_MUSIC)
		enableLog = FALSE;

	if (!(templates.LogFlags & xsc->action))
		enableLog = FALSE;

	if (enableLog && db_get_b(xsc->hContact, MODULE, "EnableXLogging", 1) && CheckMsgWnd(xsc->hContact))
		LogToMessageWindow(xsc, FALSE);

	if (opt.Log)
		LogChangeToFile(xsc);

	FreeXSC(xsc);
}

TCHAR *GetDefaultXstatusName(int statusID, char *szProto, TCHAR *buff, int bufflen)
{
	TCHAR nameBuff[64];
	buff[0] = 0;

	CUSTOM_STATUS xstatus = {0};
	xstatus.cbSize = sizeof(CUSTOM_STATUS);
	xstatus.flags = CSSF_MASK_NAME | CSSF_DEFAULT_NAME | CSSF_TCHAR;
	xstatus.ptszName = nameBuff;
	xstatus.wParam = (WPARAM *)&statusID;
	if (!CallProtoService(szProto, PS_GETCUSTOMSTATUSEX, 0, (LPARAM)&xstatus)) {
		_tcsncpy(buff, TranslateTS(nameBuff), bufflen);
		buff[bufflen - 1] = 0;
	}

	return buff;
}

TCHAR *GetIcqXStatus(MCONTACT hContact, char *szProto, char *szValue, TCHAR *buff, int bufflen)
{
	DBVARIANT dbv;
	buff[0] = 0;

	int statusID = db_get_b(hContact, szProto, "XStatusId", -1);
	if (statusID != -1) {
		if (!db_get_ts(hContact, szProto, szValue, &dbv)) {
			if ((strcmp(szValue, "XStatusName") == 0) && dbv.ptszVal[0] == 0)
				GetDefaultXstatusName(statusID, szProto, buff, bufflen);
			else
				_tcsncpy(buff, dbv.ptszVal, bufflen);

			buff[bufflen - 1] = 0;
			db_free(&dbv);
		}
	}

	return buff;
}

TCHAR *GetJabberAdvStatusText(MCONTACT hContact, char *szProto, char *szSlot, char *szValue, TCHAR *buff, int bufflen)
{
	DBVARIANT dbv;
	char szSetting[128];
	buff[0] = 0;

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/%s", szProto, szSlot, szValue);
	if (!db_get_ts(hContact, "AdvStatus", szSetting, &dbv)) {
		_tcsncpy(buff, dbv.ptszVal, bufflen);
		buff[bufflen - 1] = 0;
		db_free(&dbv);
	}

	return buff;
}

void LogXstatusChange(MCONTACT hContact, char *szProto, int xstatusType, TCHAR *stzTitle, TCHAR *stzText)
{
	XSTATUSCHANGE *xsc = NewXSC(hContact, szProto, xstatusType, NOTIFY_OPENING_ML, stzTitle[0] ? mir_tstrdup(stzTitle) : NULL, stzText[0] ? mir_tstrdup(stzText) : NULL);

	LogToMessageWindow(xsc, TRUE);
	FreeXSC(xsc);
}

void AddEventThread(void *arg)
{
	MCONTACT hContact = (MCONTACT)arg;
	TCHAR stzTitle[MAX_TITLE_LEN], stzText[MAX_TEXT_LEN];

	char *szProto = GetContactProto(hContact);
	if (szProto == NULL)
		return;

	if (ProtoServiceExists(szProto, JS_PARSE_XMPP_URI)) {
		GetJabberAdvStatusText(hContact, szProto, "mood", "title", stzTitle, SIZEOF(stzTitle));
		if (stzTitle[0]) {
			GetJabberAdvStatusText(hContact, szProto, "mood", "text", stzText, SIZEOF(stzText));
			LogXstatusChange(hContact, szProto, TYPE_JABBER_MOOD, stzTitle, stzText);
		}

		GetJabberAdvStatusText(hContact, szProto, "activity", "title", stzTitle, SIZEOF(stzTitle));
		if (stzTitle[0]) {
			GetJabberAdvStatusText(hContact, szProto, "activity", "text", stzText, SIZEOF(stzText));
			LogXstatusChange(hContact, szProto, TYPE_JABBER_ACTIVITY, stzTitle, stzText);
		}
	}
	else {
		GetIcqXStatus(hContact, szProto, "XStatusName", stzTitle, SIZEOF(stzTitle));
		if (stzTitle[0]) {
			GetIcqXStatus(hContact, szProto, "XStatusMsg", stzText, SIZEOF(stzText));
			LogXstatusChange(hContact, szProto, TYPE_ICQ_XSTATUS, stzTitle, stzText);
		}
	}
}

int OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	MessageWindowEventData *mwed = (MessageWindowEventData *)lParam;

	if (mwed->uType == MSG_WINDOW_EVT_CLOSE && !opt.KeepInHistory) {
		RemoveLoggedEvents(mwed->hContact);
		return 0;
	}

	if (opt.EnableLogging && (mwed->uType == MSG_WINDOW_EVT_OPEN) && (templates.LogFlags & NOTIFY_OPENING_ML) && (db_get_b(mwed->hContact, MODULE, "EnableLogging", 1) == 1))
		mir_forkthread(AddEventThread, (void *)mwed->hContact);

	return 0;
}
