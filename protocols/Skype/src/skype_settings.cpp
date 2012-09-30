#include "skype_proto.h"

BYTE CSkypeProto::GetSettingByte(HANDLE hContact, const char *setting, BYTE errorValue)
{
	return ::DBGetContactSettingByte(hContact, this->m_szModuleName, setting, errorValue);
}

BYTE CSkypeProto::GetSettingByte(const char *setting, BYTE errorValue)
{
	return this->GetSettingByte(NULL, setting, errorValue);
}

WORD CSkypeProto::GetSettingWord(HANDLE hContact, const char *setting, WORD errorValue)
{
	return ::DBGetContactSettingWord(hContact, this->m_szModuleName, setting, errorValue);
}

WORD CSkypeProto::GetSettingWord(const char *setting, WORD errorValue)
{
	return this->GetSettingWord(NULL, setting, errorValue);
}

DWORD CSkypeProto::GetSettingDword(HANDLE hContact, const char *setting, DWORD errorValue)
{
	return ::DBGetContactSettingDword(hContact, this->m_szModuleName, setting, errorValue);
}

DWORD CSkypeProto::GetSettingDword(const char *setting, DWORD errorValue)
{
	return this->GetSettingDword(NULL, setting, errorValue);
}

TCHAR* CSkypeProto::GetSettingString(HANDLE hContact, const char *setting, TCHAR* errorValue)
{
	DBVARIANT dbv;
	TCHAR* result = NULL;

	if ( !::DBGetContactSettingWString(hContact, this->m_szModuleName, setting, &dbv))
	{
		result = mir_wstrdup(dbv.pwszVal);
		DBFreeVariant(&dbv);
	}

	return result != NULL ? result : errorValue;
}

TCHAR* CSkypeProto::GetSettingString(const char *setting, TCHAR* errorValue)
{
	return this->GetSettingString(NULL, setting, errorValue);
}

TCHAR* CSkypeProto::GetDecodeSettingString(HANDLE hContact, const char *setting, TCHAR* errorValue)
{
	TCHAR* result = this->GetSettingString(hContact, setting, errorValue);

	CallService(
		MS_DB_CRYPT_DECODESTRING,
		wcslen(result) + 1,
		reinterpret_cast<LPARAM>(result));

	return result;
}

TCHAR* CSkypeProto::GetDecodeSettingString(const char *setting, TCHAR* errorValue)
{
	return this->GetDecodeSettingString(NULL, setting, errorValue);
}


bool CSkypeProto::SetSettingByte(HANDLE hContact, const char *setting, BYTE value)
{
	return !::DBWriteContactSettingByte(hContact, this->m_szModuleName, setting, value);
}

bool CSkypeProto::SetSettingByte(const char *setting, BYTE errorValue)
{
	return this->SetSettingByte(NULL, setting, errorValue);
}

bool CSkypeProto::SetSettingWord(HANDLE hContact, const char *setting, WORD value)
{
	return !::DBWriteContactSettingWord(hContact, this->m_szModuleName, setting, value);
}

bool CSkypeProto::SetSettingWord(const char *setting, WORD value)
{
	return this->SetSettingWord(NULL, setting, value);
}

bool CSkypeProto::SetSettingDword(HANDLE hContact, const char *setting, DWORD value)
{
	return !::DBWriteContactSettingDword(hContact, this->m_szModuleName, setting, value);
}

bool CSkypeProto::SetSettingDword(const char *setting, DWORD value)
{
	return this->SetSettingDword(NULL, setting, value);
}

bool CSkypeProto::SetSettingString(HANDLE hContact, const char *szSetting, TCHAR* value)
{
	return !::DBWriteContactSettingWString(hContact, this->m_szModuleName, szSetting, value);
}

bool CSkypeProto::SetSettingString(const char *szSetting, TCHAR* value)
{
	return this->SetSettingString(NULL, szSetting, value);
}

bool CSkypeProto::SetDecodeSettingString(HANDLE hContact, const char *setting, TCHAR* value)
{
	TCHAR* result = mir_wstrdup(value);
	CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(result), reinterpret_cast<LPARAM>(result));
	
	return !this->SetSettingString(hContact, setting, result);
}

bool CSkypeProto::SetDecodeSettingString(const char *setting, TCHAR* value)
{
	return this->SetDecodeSettingString(NULL, setting, value);
}