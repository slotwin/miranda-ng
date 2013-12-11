/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project, 
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
#include "hdr/modern_commonheaders.h"
#include "hdr/modern_clist.h"
#include "hdr/modern_commonprototypes.h"
#include "hdr/modern_awaymsg.h"

void InsertContactIntoTree(HANDLE hContact,int status);
static ClcCacheEntry *displayNameCache;
void CListSettings_FreeCacheItemDataOption( ClcCacheEntry *pDst, DWORD flag );

int PostAutoRebuidMessage(HWND hwnd);
static int displayNameCacheSize;

BOOL CLM_AUTOREBUILD_WAS_POSTED = FALSE;
SortedList *clistCache = NULL;
char *GetProtoForContact(HANDLE hContact);
int GetStatusForContact(HANDLE hContact,char *szProto);
TCHAR *UnknownConctactTranslatedName = NULL;

void InvalidateDNCEbyPointer(HANDLE hContact,ClcCacheEntry *pdnce,int SettingType);

static int handleCompare( void* c1, void* c2 )
{
	INT_PTR p1, p2;

	ClcCacheEntry *dnce1 = (ClcCacheEntry *)c1;
	ClcCacheEntry *dnce2 = (ClcCacheEntry *)c2;

	p1 = (INT_PTR)dnce1->hContact;
	p2 = (INT_PTR)dnce2->hContact;

	if ( p1 == p2 )
		return 0;

	return (int)(p1 - p2);
}

void InitCacheAsync();
void UninitCacheAsync();

void InitDisplayNameCache(void)
{
	int i=0;
	InitCacheAsync();
	InitAwayMsgModule();
	clistCache = List_Create( 0, 50 );
	clistCache->sortFunc = handleCompare;
}

void FreeDisplayNameCache()
{
	UninitCacheAsync();
	UninitAwayMsgModule();
	if ( clistCache != NULL ) {
		for (int i=0; i < clistCache->realCount; i++) {
			pcli->pfnFreeCacheItem(( ClcCacheEntry* )clistCache->items[i] );
			mir_free_and_nil( clistCache->items[i] );
		}

		List_Destroy( clistCache ); 
		mir_free(clistCache);
		clistCache = NULL;
	}	
}

ClcCacheEntry* cliGetCacheEntry(HANDLE hContact)
{
	if ( !clistCache) return NULL;

	int idx;
	ClcCacheEntry *p;   
	if ( !List_GetIndex( clistCache, &hContact, &idx )) {	
		if (( p = pcli->pfnCreateCacheItem( hContact )) != NULL ) {
			List_Insert( clistCache, p, idx );
			pcli->pfnInvalidateDisplayNameCacheEntry( hContact );
		}
	}
	else p = (ClcCacheEntry*)clistCache->items[idx];
	pcli->pfnCheckCacheItem( p );
	return p;
}

void CListSettings_FreeCacheItemData(ClcCacheEntry *pDst)
{
	CListSettings_FreeCacheItemDataOption( pDst, CCI_ALL);
}

void CListSettings_FreeCacheItemDataOption( ClcCacheEntry *pDst, DWORD flag )
{
	if ( !pDst)
		return;

	if ( flag & CCI_NAME)
		pDst->freeName();

	if ( flag & CCI_GROUP ) 
		mir_free_and_nil(pDst->tszGroup);

	if ( flag & CCI_LINES ) {
		mir_free_and_nil(pDst->szSecondLineText);
		mir_free_and_nil(pDst->szThirdLineText);
		pDst->ssSecondLine.DestroySmileyList();
		pDst->ssThirdLine.DestroySmileyList();
	}
}

int CListSettings_GetCopyFromCache(ClcCacheEntry *pDest, DWORD flag);
int CListSettings_SetToCache(ClcCacheEntry *pSrc, DWORD flag);


void CListSettings_CopyCacheItems(ClcCacheEntry *pDst, ClcCacheEntry *pSrc, DWORD flag)
{
	if ( !pDst || !pSrc) return;
	CListSettings_FreeCacheItemDataOption(pDst, flag);

	if ( flag & CCI_NAME ) {
		pDst->isUnknown = pSrc->isUnknown;
		if (pSrc->isUnknown)
			pDst->tszName = pSrc->tszName;
		else
			pDst->tszName = mir_tstrdup(pSrc->tszName);
	}

	if ( flag & CCI_GROUP )  pDst->tszGroup = mir_tstrdup(pSrc->tszGroup);
	if ( flag & CCI_PROTO )  pDst->m_cache_cszProto = pSrc->m_cache_cszProto;
	if ( flag & CCI_STATUS ) pDst->m_cache_nStatus = pSrc->m_cache_nStatus;
	
	if ( flag & CCI_LINES ) {
		mir_free( pDst->szThirdLineText );
		pDst->szThirdLineText = mir_tstrdup( pSrc->szThirdLineText );
	
		mir_free( pDst->szSecondLineText );
		pDst->szSecondLineText = mir_tstrdup( pSrc->szSecondLineText );

		pDst->ssThirdLine  = pSrc->ssThirdLine;
		pDst->ssSecondLine = pSrc->ssSecondLine;
	}

	if ( flag & CCI_TIME)	
		pDst->hTimeZone = pSrc->hTimeZone;

	if ( flag & CCI_OTHER) {
		pDst->bIsHidden = pSrc->bIsHidden;
		pDst->m_cache_nNoHiddenOffline = pSrc->m_cache_nNoHiddenOffline;
		pDst->m_cache_bProtoNotExists = pSrc->m_cache_bProtoNotExists;

		pDst->m_cache_nHiddenSubcontact = pSrc->m_cache_nHiddenSubcontact;
		pDst->i = pSrc->i;
		pDst->ApparentMode = pSrc->ApparentMode;
		pDst->NotOnList = pSrc->NotOnList;
		pDst->IdleTS = pSrc->IdleTS;
		pDst->ClcContact = pSrc->ClcContact;
		pDst->IsExpanded = pSrc->IsExpanded;
	}
}

int CListSettings_GetCopyFromCache(ClcCacheEntry *pDest, DWORD flag)
{
	if ( !pDest || !pDest->hContact)
		return -1;

	ClcCacheEntry *pSource = pcli->pfnGetCacheEntry(pDest->hContact);
	if ( !pSource)
		return -1;

	CListSettings_CopyCacheItems(pDest, pSource, flag);
	return 0;
}

int CListSettings_SetToCache(ClcCacheEntry *pSrc, DWORD flag)
{
	if ( !pSrc || !pSrc->hContact)
		return -1;

	ClcCacheEntry *pDst = pcli->pfnGetCacheEntry(pSrc->hContact);
	if ( !pDst)
		return -1;

	CListSettings_CopyCacheItems(pDst, pSrc, flag);
	return 0;
}

void cliFreeCacheItem( ClcCacheEntry *p )
{
	HANDLE hContact = p->hContact;
	TRACEVAR("cliFreeCacheItem hContact = %d",hContact);
	p->freeName();
	mir_free_and_nil(p->tszGroup);
	mir_free_and_nil(p->szSecondLineText);
	mir_free_and_nil(p->szThirdLineText);
	p->ssSecondLine.DestroySmileyList();
	p->ssThirdLine.DestroySmileyList();
}

void cliCheckCacheItem(ClcCacheEntry *pdnce)
{
	if (pdnce == NULL)
		return;
	
	if (pdnce->hContact == NULL) { //selfcontact
		if ( !pdnce->tszName)
			pdnce->getName();
		return;
	}

	if (pdnce->m_cache_cszProto == NULL && pdnce->m_cache_bProtoNotExists == FALSE) {
		pdnce->m_cache_cszProto = GetProtoForContact(pdnce->hContact);
		if (pdnce->m_cache_cszProto == NULL) 
			pdnce->m_cache_bProtoNotExists = FALSE;
		else if (pdnce->m_cache_cszProto && pdnce->tszName)
			pdnce->freeName();
	}

	if (pdnce->tszName == NULL)
		pdnce->getName();

	else if (pdnce->isUnknown && pdnce->m_cache_cszProto && pdnce->m_cache_bProtoNotExists == TRUE && g_flag_bOnModulesLoadedCalled) {
		if (CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)pdnce->m_cache_cszProto) == 0) {
			pdnce->m_cache_bProtoNotExists = FALSE;						
			pdnce->getName();
		}
	}

	if (pdnce___GetStatus( pdnce ) == 0) //very strange look status sort is broken let always reread status
		pdnce___SetStatus( pdnce , GetStatusForContact(pdnce->hContact,pdnce->m_cache_cszProto));

	if (pdnce->tszGroup == NULL) {
		DBVARIANT dbv = {0};
		if ( !db_get_ts(pdnce->hContact,"CList","Group",&dbv)) {
			pdnce->tszGroup = mir_tstrdup(dbv.ptszVal);
			db_free(&dbv);
		}
		else pdnce->tszGroup = mir_tstrdup(_T(""));
	}

	if (pdnce->bIsHidden == -1)
		pdnce->bIsHidden = db_get_b(pdnce->hContact,"CList","Hidden",0);

	pdnce->m_cache_nHiddenSubcontact = g_szMetaModuleName && db_get_b(pdnce->hContact,g_szMetaModuleName,"IsSubcontact",0);

	if (pdnce->m_cache_nNoHiddenOffline == -1)
		pdnce->m_cache_nNoHiddenOffline = db_get_b(pdnce->hContact,"CList","noOffline",0);

	if (pdnce->IdleTS == -1)
		pdnce->IdleTS = db_get_dw(pdnce->hContact,pdnce->m_cache_cszProto,"IdleTS",0);

	if (pdnce->ApparentMode == -1)
		pdnce->ApparentMode = db_get_w(pdnce->hContact,pdnce->m_cache_cszProto,"ApparentMode",0);

	if (pdnce->NotOnList == -1)
		pdnce->NotOnList = db_get_b(pdnce->hContact,"CList","NotOnList",0);

	if (pdnce->IsExpanded == -1)
		pdnce->IsExpanded = db_get_b(pdnce->hContact,"CList","Expanded",0);

	if (pdnce->dwLastMsgTime == 0) {
		pdnce->dwLastMsgTime = db_get_dw(pdnce->hContact, "CList", "mf_lastmsg", 0);
		if (pdnce->dwLastMsgTime == 0)
			pdnce->dwLastMsgTime = CompareContacts2_getLMTime(pdnce->hContact);
	}
}

void IvalidateDisplayNameCache(DWORD mode)
{
	if ( clistCache != NULL ) 
	{
		int i;
		for ( i=0; i < clistCache->realCount; i++) 
		{
			ClcCacheEntry *pdnce = (ClcCacheEntry *)clistCache->items[i];
			if (mode&16)
			{
				InvalidateDNCEbyPointer(pdnce->hContact,pdnce,16);
			}
		}
	}
}

void InvalidateDNCEbyPointer(HANDLE hContact, ClcCacheEntry *pdnce, int SettingType)
{
	if (hContact == NULL || pdnce == NULL)
		return;
	
	if (SettingType == 16) {
		pdnce->ssSecondLine.DestroySmileyList();
		mir_free_and_nil(pdnce->szSecondLineText);
		pdnce->ssThirdLine.DestroySmileyList();
		mir_free_and_nil(pdnce->szThirdLineText);
		pdnce->ssSecondLine.iMaxSmileyHeight = 0;
		pdnce->ssThirdLine.iMaxSmileyHeight = 0;
		pdnce->hTimeZone = NULL;
		pdnce->dwLastMsgTime = 0;
		Cache_GetTimezone(NULL,pdnce->hContact);
		SettingType &= ~16;
	}

	if (SettingType >= DBVT_WCHAR) {
		pdnce->freeName();
		mir_free_and_nil(pdnce->tszGroup);
		pdnce->m_cache_cszProto = NULL;
		return;
	}

	if (SettingType == -1 || SettingType == DBVT_DELETED) {	
		pdnce->freeName();
		mir_free_and_nil(pdnce->tszGroup);
		pdnce->m_cache_cszProto = NULL;
	}
	// in other cases clear all binary cache
	else pdnce->dwLastMsgTime = 0;

	pdnce->bIsHidden = -1;
	pdnce->m_cache_nHiddenSubcontact = -1;
	pdnce->m_cache_bProtoNotExists = FALSE;
	pdnce___SetStatus(pdnce, 0);
	pdnce->IdleTS = -1;
	pdnce->ApparentMode = -1;
	pdnce->NotOnList = -1;
	pdnce->isUnknown = FALSE;
	pdnce->m_cache_nNoHiddenOffline = -1;
	pdnce->IsExpanded = -1;
}

char *GetContactCachedProtocol(HANDLE hContact)
{
	ClcCacheEntry *cacheEntry = NULL;
	cacheEntry = pcli->pfnGetCacheEntry(hContact);
	if (cacheEntry && cacheEntry->m_cache_cszProto)
		return cacheEntry->m_cache_cszProto;

	return NULL;
}

char* GetProtoForContact(HANDLE hContact)
{
	return (char*)CallService(MS_PROTO_GETCONTACTBASEACCOUNT,(WPARAM)hContact,0);
}

int GetStatusForContact(HANDLE hContact,char *szProto)
{
	return (szProto) ? (int)(db_get_w((HANDLE)hContact,szProto,"Status",ID_STATUS_OFFLINE)) : ID_STATUS_OFFLINE;
}

void ClcCacheEntry::freeName()
{
	if ( !isUnknown)
		mir_free(tszName);
	else 
		isUnknown = false;
	tszName = NULL;
}

void ClcCacheEntry::getName()
{
	freeName();

	if (m_cache_bProtoNotExists || !m_cache_cszProto) {
LBL_Unknown:
		tszName = UnknownConctactTranslatedName;
		isUnknown = true;
		return;
	}

	tszName = pcli->pfnGetContactDisplayName(hContact, GCDNF_NOCACHE);
	if ( !lstrcmp(tszName, UnknownConctactTranslatedName)) {
		mir_free(tszName);
		goto LBL_Unknown;
	}

	isUnknown = false;
}

int GetContactInfosForSort(HANDLE hContact,char **Proto,TCHAR **Name,int *Status)
{
	ClcCacheEntry *cacheEntry = NULL;
	cacheEntry = pcli->pfnGetCacheEntry(hContact);
	if (cacheEntry != NULL)
	{
		if (Proto != NULL)  *Proto = cacheEntry->m_cache_cszProto;
		if (Name != NULL)   *Name = cacheEntry->tszName;
		if (Status != NULL) *Status = pdnce___GetStatus( cacheEntry );
	}
	return (0);
};


int GetContactCachedStatus(HANDLE hContact)
{
	ClcCacheEntry *cacheEntry = NULL;
	cacheEntry = pcli->pfnGetCacheEntry(hContact);
	return pdnce___GetStatus( cacheEntry );
}

int ContactAdded(WPARAM wParam,LPARAM lParam)
{
	if ( !MirandaExiting()) {
		HANDLE hContact = (HANDLE)wParam;
		cli_ChangeContactIcon(hContact,pcli->pfnIconFromStatusMode((char*)GetContactCachedProtocol(hContact),ID_STATUS_OFFLINE,hContact),1); ///by FYR
		pcli->pfnSortContacts();
	}
	return 0;
}

int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	if (MirandaExiting() || !pcli || !clistCache || hContact == NULL)
		return 0;

	ClcCacheEntry *pdnce = pcli->pfnGetCacheEntry(hContact);
	if (pdnce == NULL) {
		TRACE("!!! Very bad pdnce not found.");
		return 0;
	}

	if (pdnce->m_cache_bProtoNotExists || !pdnce->m_cache_cszProto)
		return 0;

	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	if ( !strcmp(cws->szModule, pdnce->m_cache_cszProto)) {
		InvalidateDNCEbyPointer(hContact, pdnce, cws->value.type);

		if ( !strcmp(cws->szSetting,"IsSubcontact"))
			PostMessage(pcli->hwndContactTree,CLM_AUTOREBUILD, 0, 0);

		if ( !mir_strcmp(cws->szSetting, "Status") || wildcmp(cws->szSetting, "Status?")) {
			if (g_szMetaModuleName && !mir_strcmp(cws->szModule,g_szMetaModuleName) && mir_strcmp(cws->szSetting, "Status")) {
				int res = 0;
				if (pcli->hwndContactTree && g_flag_bOnModulesLoadedCalled) 
					res = PostAutoRebuidMessage(pcli->hwndContactTree);

				if ((db_get_w(NULL,"CList","SecondLineType",SETTING_SECONDLINE_TYPE_DEFAULT) == TEXT_STATUS_MESSAGE || db_get_w(NULL,"CList","ThirdLineType",SETTING_THIRDLINE_TYPE_DEFAULT) == TEXT_STATUS_MESSAGE)  && pdnce->hContact && pdnce->m_cache_cszProto)
					amRequestAwayMsg(hContact);  

				return 0;
			}

			if (pdnce->bIsHidden != 1) {		
				pdnce___SetStatus( pdnce , cws->value.wVal ); //dont use direct set
				if (cws->value.wVal == ID_STATUS_OFFLINE)
					if (g_CluiData.bRemoveAwayMessageForOffline)
						db_set_s(hContact,"CList","StatusMsg","");

				if ((db_get_w(NULL,"CList","SecondLineType",0) == TEXT_STATUS_MESSAGE || db_get_w(NULL,"CList","ThirdLineType",0) == TEXT_STATUS_MESSAGE)  && pdnce->hContact && pdnce->m_cache_cszProto)
					amRequestAwayMsg(hContact);  

				pcli->pfnClcBroadcast(INTM_STATUSCHANGED, wParam, 0);
				cli_ChangeContactIcon(hContact, pcli->pfnIconFromStatusMode(cws->szModule, cws->value.wVal, hContact), 0); //by FYR
				pcli->pfnSortContacts();
			}
			else {
				if ( !(!mir_strcmp(cws->szSetting, "LogonTS") || !mir_strcmp(cws->szSetting, "TickTS") || !mir_strcmp(cws->szSetting, "InfoTS")))
					pcli->pfnSortContacts();

				return 0;
			}
		}
	}

	if ( !strcmp(cws->szModule,"CList")) {
		//name is null or (setting is myhandle)
		if ( !strcmp(cws->szSetting,"Rate"))
			pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);

		else if (pdnce->tszName == NULL || !strcmp(cws->szSetting,"MyHandle"))
			InvalidateDNCEbyPointer(hContact,pdnce,cws->value.type);

		else if ( !strcmp(cws->szSetting,"Group")) 
			InvalidateDNCEbyPointer(hContact,pdnce,cws->value.type);

		else if ( !strcmp(cws->szSetting,"Hidden")) {
			InvalidateDNCEbyPointer(hContact,pdnce,cws->value.type);		
			if (cws->value.type == DBVT_DELETED || cws->value.bVal == 0) {
				char *szProto = GetContactProto((HANDLE)wParam);
				cli_ChangeContactIcon(hContact,pcli->pfnIconFromStatusMode(szProto, 
					szProto == NULL ? ID_STATUS_OFFLINE : db_get_w(hContact,szProto,"Status",ID_STATUS_OFFLINE), hContact),1);  //by FYR
			}
			pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
		}
		else if ( !strcmp(cws->szSetting,"noOffline")) {
			InvalidateDNCEbyPointer(hContact,pdnce,cws->value.type);		
			pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
		}
	}
	else if ( !strcmp(cws->szModule,"Protocol")) {
		if ( !strcmp(cws->szSetting,"p")) {
			InvalidateDNCEbyPointer(hContact,pdnce,cws->value.type);	
			char *szProto = (cws->value.type == DBVT_DELETED) ? NULL : cws->value.pszVal;
			cli_ChangeContactIcon(hContact,pcli->pfnIconFromStatusMode(szProto, 
				szProto == NULL ? ID_STATUS_OFFLINE : db_get_w(hContact,szProto,"Status",ID_STATUS_OFFLINE), hContact), 0);
		}
	}

	return 0;
}

int PostAutoRebuidMessage(HWND hwnd)
{
	if ( !CLM_AUTOREBUILD_WAS_POSTED)
		CLM_AUTOREBUILD_WAS_POSTED = PostMessage(hwnd,CLM_AUTOREBUILD, 0, 0);
	return CLM_AUTOREBUILD_WAS_POSTED;
}

int OnLoadLangpack(WPARAM, LPARAM)
{
	UnknownConctactTranslatedName = TranslateT("(Unknown Contact)");
	return 0;
}