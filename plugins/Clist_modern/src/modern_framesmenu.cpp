#include "hdr/modern_commonheaders.h"
#include "hdr/modern_commonprototypes.h"

//========================== Frames
HANDLE hFrameMenuObject;

//contactmenu exec param(ownerdata)
//also used in checkservice
typedef struct{
	char *szServiceName;
	int Frameid;
	INT_PTR param1;
}
	FrameMenuExecParam,*lpFrameMenuExecParam;

static INT_PTR AddContextFrameMenuItem(WPARAM wParam, LPARAM lParam)
{
	CLISTMENUITEM *mi = (CLISTMENUITEM*)lParam;

	TMO_MenuItem tmi;
	if ( !pcli->pfnConvertMenu(mi, &tmi))
		return NULL;

	tmi.root = (mi->flags & CMIF_ROOTHANDLE) ? mi->hParentMenu : NULL;

	lpFrameMenuExecParam fmep = (lpFrameMenuExecParam)mir_alloc(sizeof(FrameMenuExecParam));
	if (fmep == NULL)
		return 0;

	memset(fmep, 0, sizeof(FrameMenuExecParam));
	fmep->szServiceName = mir_strdup(mi->pszService);
	fmep->Frameid = mi->popupPosition;
	fmep->param1 = (INT_PTR)mi->pszContactOwner;
	tmi.ownerdata = fmep;
	return CallService(MO_ADDNEWMENUITEM, (WPARAM)hFrameMenuObject, (LPARAM)&tmi);
}

static INT_PTR RemoveContextFrameMenuItem(WPARAM wParam, LPARAM lParam)
{
	lpFrameMenuExecParam fmep = (lpFrameMenuExecParam)CallService(MO_MENUITEMGETOWNERDATA, wParam, lParam);
	if (fmep != NULL) {
		if (fmep->szServiceName != NULL)
			mir_free(fmep->szServiceName);
		mir_free(fmep);
	}

	if (lParam != 1)
		CallService(MO_REMOVEMENUITEM, wParam, 0);

	return 0;
}

//called with:
//wparam - ownerdata
//lparam - lparam from winproc
INT_PTR FrameMenuExecService(WPARAM wParam, LPARAM lParam)
{
	lpFrameMenuExecParam fmep = (lpFrameMenuExecParam)wParam;
	if (fmep ==NULL)
		return -1;

	CallService(fmep->szServiceName, lParam, fmep->param1);
	return 0;
}

//true - ok,false ignore
INT_PTR FrameMenuCheckService(WPARAM wParam, LPARAM lParam)
{
	PCheckProcParam pcpp = (PCheckProcParam)wParam;
	if (pcpp == NULL)
		return FALSE;

	TMO_MenuItem mi;
	if ( CallService(MO_GETMENUITEM, (WPARAM)pcpp->MenuItemHandle, (LPARAM)&mi) == 0) {
		lpFrameMenuExecParam fmep = (lpFrameMenuExecParam)mi.ownerdata;
		if (fmep != NULL) {
			//pcpp->wParam  -  frameid
			if (((WPARAM)fmep->Frameid == pcpp->wParam) || fmep->Frameid == -1)
				return TRUE;
		}
	}
	return FALSE;
}

static INT_PTR ContextFrameMenuNotify(WPARAM wParam, LPARAM lParam)
{
	NotifyEventHooks(g_CluiData.hEventPreBuildFrameMenu, wParam, lParam);
	return 0;
}

static INT_PTR BuildContextFrameMenu(WPARAM wParam,LPARAM lParam)
{
	ListParam param = { 0 };
	param.MenuObjectHandle = hFrameMenuObject;
	param.wParam = wParam;
	param.lParam = lParam;

	HMENU hMenu = CreatePopupMenu();
	ContextFrameMenuNotify(wParam, -1);
	CallService(MO_BUILDMENU, (WPARAM)hMenu, (LPARAM)&param);
	return (INT_PTR)hMenu;
}

//========================== Frames end
bool InternalGenMenuModule = FALSE;

int MeasureItemProxy(WPARAM wParam, LPARAM lParam)
{
	if (InternalGenMenuModule) {
		int val = CallService(MS_INT_MENUMEASUREITEM, wParam, lParam);
		if (val)
			return val;
	}
	return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
}

int DrawItemProxy(WPARAM wParam, LPARAM lParam)
{
	if (InternalGenMenuModule) {
		int val = CallService(MS_INT_MENUDRAWITEM, wParam, lParam);
		if (val)
			return val;
	}
	return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
}

int ProcessCommandProxy(WPARAM wParam, LPARAM lParam)
{
	if (InternalGenMenuModule) {
		int val = CallService(MS_INT_MENUPROCESSCOMMAND, wParam, lParam);
		if (val)
			return val;
	}
	return CallService(MS_CLIST_MENUPROCESSCOMMAND,wParam,lParam);
}

int ModifyMenuItemProxy(WPARAM wParam, LPARAM lParam)
{
	if (InternalGenMenuModule) {
		int val = CallService(MS_INT_MODIFYMENUITEM, wParam, lParam);
		if (val)
			return val;
	}
	return CallService(MS_CLIST_MODIFYMENUITEM,wParam,lParam);
}

int InitFramesMenus(void)
{
	if ( !ServiceExists(MO_REMOVEMENUOBJECT)) {
 		InitCustomMenus();
		InternalGenMenuModule = TRUE;
	}

	if ( ServiceExists(MO_REMOVEMENUOBJECT)) {
		CreateServiceFunction("FrameMenuExecService",FrameMenuExecService);
		CreateServiceFunction("FrameMenuCheckService",FrameMenuCheckService);

		CreateServiceFunction(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,RemoveContextFrameMenuItem);
		CreateServiceFunction("CList/AddContextFrameMenuItem",AddContextFrameMenuItem);
		CreateServiceFunction(MS_CLIST_MENUBUILDFRAMECONTEXT,BuildContextFrameMenu);
		CreateServiceFunction(MS_CLIST_FRAMEMENUNOTIFY,ContextFrameMenuNotify);

		//frame menu object
		TMenuParam tmp = { sizeof(tmp) };
		tmp.CheckService = "FrameMenuCheckService";
		tmp.ExecService = "FrameMenuExecService";
		tmp.name = "FrameMenu";
		hFrameMenuObject = (HANDLE)CallService(MO_CREATENEWMENUOBJECT, 0, (LPARAM)&tmp);
	}
	return 0;
}

int UnitFramesMenu()
{
	return 0;
}