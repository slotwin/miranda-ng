/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.

Copyright (c) 2012-2013 Miranda NG Team
Copyright (c) 2006-2012 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"
#include "msn_proto.h"

HANDLE CMsnProto::MSN_HContactFromEmail(const char* wlid, const char* msnNick, bool addIfNeeded, bool temporary)
{
	HANDLE hContact = NULL;

	char* szEmail;
	parseWLID(NEWSTR_ALLOCA(wlid), NULL, &szEmail, NULL);

	MsnContact *msc = Lists_Get(szEmail);
	if (msc && msc->hContact) hContact = msc->hContact;

	if (hContact == NULL && addIfNeeded)
	{
		hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)m_szModuleName);
		setString(hContact, "e-mail", szEmail);
		setStringUtf(hContact, "Nick", msnNick ? msnNick : wlid);
		if (temporary)
			db_set_b(hContact, "CList", "NotOnList", 1);

		Lists_Add(0, NETID_MSN, wlid, hContact);
	}

	return hContact;
}


void CMsnProto::MSN_SetContactDb(HANDLE hContact, const char *szEmail)
{
	MsnContact *cont = Lists_Get(szEmail);
	const int listId = cont->list;

	if (listId & LIST_FL)
	{
		if (db_get_b(hContact, "CList", "NotOnList", 0) == 1)
		{
			db_unset(hContact, "CList", "NotOnList");
			db_unset(hContact, "CList", "Hidden");
		}

		if (listId & (LIST_BL | LIST_AL))
		{
			WORD tApparentMode = getWord(hContact, "ApparentMode", 0);
			if ((listId & LIST_BL) && tApparentMode == 0)
				setWord(hContact, "ApparentMode", ID_STATUS_OFFLINE);
			else if ((listId & LIST_AL) && tApparentMode != 0)
				delSetting(hContact, "ApparentMode");
		}

		if (cont->netId == NETID_MOB)
		{
			setWord(hContact, "Status", ID_STATUS_ONTHEPHONE);
			setString(hContact, "MirVer", "SMS");
		}
	}
	if (listId & LIST_LL)
		setByte(hContact, "LocalList", 1);
	else
		delSetting(hContact, "LocalList");

}


void CMsnProto::AddDelUserContList(const char* email, const int list, const int netId, const bool del)
{
	char buf[512];
	size_t sz;

	if (list < LIST_RL)
	{
		const char* dom = strchr(email, '@');
		if (dom == NULL)
		{
			sz = mir_snprintf(buf, sizeof(buf),
				"<ml><t><c n=\"%s\" l=\"%d\"/></t></ml>",
				email, list);
		}
		else
		{
			*(char*)dom = 0;
			sz = mir_snprintf(buf, sizeof(buf),
				"<ml><d n=\"%s\"><c n=\"%s\" l=\"%d\" t=\"%d\"/></d></ml>",
				dom+1, email, list, netId);
			*(char*)dom = '@';
		}
		msnNsThread->sendPacket(del ? "RML" : "ADL", "%d\r\n%s", sz, buf);
	}

	if (del)
		Lists_Remove(list, email);
	else
		Lists_Add(list, netId, email);
}


/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddUser - adds a e-mail address to one of the MSN server lists

bool CMsnProto::MSN_AddUser(HANDLE hContact, const char* email, int netId, int flags, const char *msg)
{
	bool needRemove     = (flags & LIST_REMOVE) != 0;
	bool leaveHotmail   = (flags & LIST_REMOVENH) == LIST_REMOVENH;
	flags &= 0xFF;

	if (needRemove != Lists_IsInList(flags, email))
		return true;


	bool res = false;
	if (flags == LIST_FL)
	{
		if (needRemove)
		{
			if (hContact == NULL)
			{
				hContact = MSN_HContactFromEmail(email);
				if (hContact == NULL) return false;
			}

			char id[MSN_GUID_LEN];
			if (!getStaticString(hContact, "ID", id, sizeof(id)))
			{
				int netId = Lists_GetNetId(email);
				if (leaveHotmail)
					res = MSN_ABAddRemoveContact(id, netId, false);
				else
					res = MSN_ABAddDelContactGroup(id , NULL, "ABContactDelete");
				if (res) AddDelUserContList(email, flags, netId, true);

				delSetting(hContact, "GroupID");
				delSetting(hContact, "ID");
				MSN_RemoveEmptyGroups();
			}
		}
		else
		{
			DBVARIANT dbv = {0};
			if (!strcmp(email, MyOptions.szEmail))
				getStringUtf("Nick", &dbv);

			unsigned res1 = MSN_ABContactAdd(email, dbv.pszVal, netId, msg, false);
			if (netId == NETID_MSN && res1 == 2)
			{
				netId = NETID_LCS;
				res = MSN_ABContactAdd(email, dbv.pszVal, netId, msg, false) == 0;
			}
			else if (netId == NETID_MSN && res1 == 3)
			{
				char szContactID[100];
				hContact = MSN_HContactFromEmail(email);
				if (getStaticString(hContact, "ID", szContactID, sizeof(szContactID)) == 0)
				{
					MSN_ABAddRemoveContact(szContactID, netId, true);
					res = true;
				}
			}

			else
				res = (res1 == 0);

			if (res)
			{
				DBVARIANT dbv;
				if (!db_get_utf(hContact, "CList", "Group", &dbv))
				{
					MSN_MoveContactToGroup(hContact, dbv.pszVal);
					db_free(&dbv);
				}

				char szContactID[100];
				if (getStaticString(hContact, "ID", szContactID, sizeof(szContactID)) == 0)
				{
					MSN_ABFind("ABFindByContacts", szContactID);
				}

				MSN_SharingFindMembership(true);
				AddDelUserContList(email, flags, netId, false);
			}
			else
			{
				if (netId == 1 && strstr(email, "@yahoo.com") != 0)
					MSN_FindYahooUser(email);
			}
			db_free(&dbv);
		}
	}
	else if (flags == LIST_LL)
	{
		if (needRemove)
			Lists_Remove(LIST_LL, email);
		else
			Lists_Add(LIST_LL, NETID_MSN, email);
	}
	else
	{
		if (netId == 0) netId = Lists_GetNetId(email);
		res = MSN_SharingAddDelMember(email, flags, netId, needRemove ? "DeleteMember" : "AddMember");
//		if (res || (flags & LIST_RL))
			AddDelUserContList(email, flags, netId, needRemove);
		if ((flags & LIST_BL) && !needRemove)
		{
			ThreadData* thread =  MSN_GetThreadByContact(email, SERVER_SWITCHBOARD);
			if (thread) thread->sendTerminate();
		}
		if ((flags & LIST_PL) && needRemove)
		{
			MSN_AddUser(hContact, email, netId, LIST_RL);
		}
	}
	return res;
}


void CMsnProto::MSN_FindYahooUser(const char* email)
{
	const char* dom = strchr(email, '@');
	if (dom)
	{
		char buf[512];
		size_t sz;

		*(char*)dom = '\0';
		sz = mir_snprintf(buf, sizeof(buf), "<ml><d n=\"%s\"><c n=\"%s\"/></d></ml>", dom+1, email);
		*(char*)dom = '@';
		msnNsThread->sendPacket("FQY", "%d\r\n%s", sz, buf);
	}
}

bool CMsnProto::MSN_RefreshContactList(void)
{
	Lists_Wipe();
	Lists_Populate();

	if (!MSN_SharingFindMembership()) return false;

	if (m_iDesiredStatus == ID_STATUS_OFFLINE) return false;

	if (!MSN_ABFind("ABFindContactsPaged", NULL)) return false;

	if (m_iDesiredStatus == ID_STATUS_OFFLINE) return false;

	MSN_CleanupLists();

	if (m_iDesiredStatus == ID_STATUS_OFFLINE) return false;

	msnLoggedIn = true;

	MSN_CreateContList();
	MSN_StoreGetProfile();
	return true;
}