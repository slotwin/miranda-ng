/*
UserinfoEx plugin for Miranda IM

Copyright:
� 2006-2010 DeathAxe, Yasnovidyashii, Merlin, K. Romanov, Kreol

part of this code based on:
Miranda IM Country Flags Plugin Copyright �2006-2007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef  _UINFOEX_FLAGS_H_INCLUDED_
#define  _UINFOEX_FLAGS_H_INCLUDED_

#define EXTRAIMAGE_REFRESHDELAY		100		/* time for which setting changes are buffered */
#define STATUSICON_REFRESHDELAY		100		/* time for which setting changes are buffered */

typedef struct _FLAGSOPTIONS 
{
	BYTE	bShowExtraImgFlag;
	BYTE	bUseUnknownFlag;
	BYTE	bShowStatusIconFlag;
} FLAGSOPTIONS, *LPFLAGSOPTIONS;

extern FLAGSOPTIONS	gFlagsOpts;

class MsgWndData {
	public:
		HANDLE	m_hContact;
		HWND	m_hwnd;
		int		m_countryID;
		
		MsgWndData(HWND hwnd, HANDLE hContact);
		~MsgWndData();

		void FlagsIconSet();
		void FlagsIconUnset();
		void FlagsIconUpdate() {
			gFlagsOpts.bShowStatusIconFlag ? FlagsIconSet():FlagsIconUnset();
		};
		void ContryIDchange(int ID) {
			m_countryID = ID; FlagsIconUpdate();
		};
};

class IconList {
	public:
		int            m_ID;
		HANDLE         m_hIcon;		//register
		BYTE           m_TypeFlag;
		StatusIconData m_StatusIconData;

		IconList(StatusIconData* sid);
//		IconList(HWND hwnd, HANDLE hContact);
		~IconList();

};

typedef void (CALLBACK *BUFFEREDPROC)(LPARAM lParam);
#ifdef _DEBUG
	void _CallFunctionBuffered(BUFFEREDPROC pfnBuffProc,const char *pszProcName,LPARAM lParam,BOOL fAccumulateSameParam,UINT uElapse);
	#define CallFunctionBuffered(proc,param,acc,elapse) _CallFunctionBuffered(proc,#proc,param,acc,elapse)
#else
	void _CallFunctionBuffered(BUFFEREDPROC pfnBuffProc,LPARAM lParam,BOOL fAccumulateSameParam,UINT uElapse);
	#define CallFunctionBuffered(proc,param,acc,elapse) _CallFunctionBuffered(proc,param,acc,elapse)
#endif

void EnsureExtraImages();
void SvcFlagsEnableExtraIcons(BYTE bEnable, BYTE bUpdateDB);
void CALLBACK UpdateStatusIcons(LPARAM lParam);

void SvcFlagsLoadModule();
void SvcFlagsOnModulesLoaded();
void SvcFlagsUnloadModule();

#endif /* _UINFOEX_FLAGS_H_INCLUDED_ */