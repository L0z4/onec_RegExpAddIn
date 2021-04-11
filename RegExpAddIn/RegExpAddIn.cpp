
#include "stdafx.h"

#pragma comment(lib, "ws2_32.lib")


#if defined( __linux__ ) || defined(__APPLE__)
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <iconv.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <wchar.h>
#include <iostream>
#include <string>
#include <regex>
#include "RegExpAddIn.h"

#define TIME_LEN 65

#define BASE_ERRNO     7

static const wchar_t *g_PropNames[] = {L"isCaseSensitive", L"Last" };
static const wchar_t *g_MethodNames[] = {L"ShowMessageBox", L"Sleep", L"Match", L"Replace", L"Search", L"GetSearchResult", L"ClipboardCopy",
		 L"Last" };

static const wchar_t *g_PropNamesRu[] = {L"���������������������", L"Last" };
static const wchar_t *g_MethodNamesRu[] = {L"�����������������", L"�����", L"���������", L"��������", L"�����", L"���������������",  L"���������������",
		 L"Last" };

static const wchar_t g_kClassNames[] = L"CAddInNative";   
static IAddInDefBase *pAsyncEvent = NULL;

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static WcharWrapper s_names(g_kClassNames);
//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
    if(!*pInterface)
    {
        *pInterface= new CAddInNative;
        return (long)*pInterface;
    }
    return 0;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
    g_capabilities = capabilities;
    return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
    if(!*pIntf)
        return -1;

    delete *pIntf;
    *pIntf = 0;
    return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
    return s_names;
}
//---------------------------------------------------------------------------//



// CAddInNative
//---------------------------------------------------------------------------//
CAddInNative::CAddInNative()
{
    m_iMemory = 0;
    m_iConnect = 0;

}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection)
{ 
	
	wstr_regex = L"";

    m_iConnect = (IAddInDefBase*)pConnection;
	isCaseSensitive = true;
	//if (m_iConnect)
	//	m_iConnect->SetEventBufferDepth(5);

    return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{ 
    // Component should put supported component technology version 
    // This component supports 2.0 version
    return 2000; 
}
//---------------------------------------------------------------------------//
void CAddInNative::Done()
{

}
/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{ 
    const wchar_t *wsExtension = L"RegExp";
    int iActualSize = ::wcslen(wsExtension) + 1;
    WCHAR_T* dest = 0;

    if (m_iMemory)
    {
        if(m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
            ::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
        return true;
    }

    return false; 
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{ 
    // You may delete next lines and add your own implementation code here
    return ePropLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{ 
    long plPropNum = -1;
    wchar_t* propName = 0;

    ::convFromShortWchar(&propName, wsPropName);
    plPropNum = findName(g_PropNames, propName, ePropLast);

    if (plPropNum == -1)
        plPropNum = findName(g_PropNamesRu, propName, ePropLast);

    delete[] propName;

    return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{ 
    if (lPropNum >= ePropLast)
        return NULL;

    wchar_t *wsCurrentName = NULL;
    WCHAR_T *wsPropName = NULL;
    int iActualSize = 0;

    switch(lPropAlias)
    {
    case 0: // First language
        wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
        break;
    case 1: // Second language
        wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
        break;
    default:
        return 0;
    }
    
    iActualSize = wcslen(wsCurrentName) + 1;

    if (m_iMemory && wsCurrentName)
    {
        if (m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))
            ::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
    }

    return wsPropName;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{ 
    switch(lPropNum)
    {
	case ePropCaseSesitive:
		TV_VT(pvarPropVal) = VTYPE_BOOL;
		pvarPropVal->bVal = isCaseSensitive;
		break;

    default:
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant *varPropVal)
{ 
    switch(lPropNum)
    { 
	case ePropCaseSesitive:
		if (TV_VT(varPropVal) != VTYPE_BOOL)
			return false;
		isCaseSensitive = varPropVal->bVal;
    default:
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum)
{ 
    switch(lPropNum)
    { 
	case ePropCaseSesitive:
		return true;
    default:
        return false;
    }

    return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
    switch(lPropNum)
    { 
	case ePropCaseSesitive:
		return true;
    default:
        return false;
    }

    return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods()
{ 
    return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{ 
    long plMethodNum = -1;
    wchar_t* name = 0;

    ::convFromShortWchar(&name, wsMethodName);

    plMethodNum = findName(g_MethodNames, name, eMethLast);

    if (plMethodNum == -1)
        plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

    delete[] name;

    return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{ 
    if (lMethodNum >= eMethLast)
        return NULL;

    wchar_t *wsCurrentName = NULL;
    WCHAR_T *wsMethodName = NULL;
    int iActualSize = 0;

    switch(lMethodAlias)
    {
    case 0: // First language
        wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
        break;
    case 1: // Second language
        wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
        break;
    default: 
        return 0;
    }

    iActualSize = wcslen(wsCurrentName) + 1;

    if (m_iMemory && wsCurrentName)
    {
        if(m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))
            ::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
    }

    return wsMethodName;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{ 
    switch(lMethodNum)
    { 
	case eSleep:
		return 1;
	case eMatch:
		return 2;
	case eReplace:
		return 3;
	case eSearch:
		return 2;
	case eGetSearchResult:
		return 1;
    case eClipboardCopy:
        return 2;
    default:
        return 0;
    }
    
    return 0;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
                        tVariant *pvarParamDefValue)
{ 
    TV_VT(pvarParamDefValue)= VTYPE_EMPTY;

    switch(lMethodNum)
    { 
    case eMethShowMsgBox:
	case eReplace:
	case eSleep:
        // There are no parameter values by default 
        break;
    default:
        return false;
    }

    return false;
} 
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum)
{ 
    switch(lMethodNum)
    { 
	case eMatch:
		return true;
	case eReplace:
		return true;
	case eSearch:
		return true;
	case eGetSearchResult:
		return true;
    case eClipboardCopy:
        return true;
    default:
        return false;
    }

    return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum,
                    tVariant* paParams, const long lSizeArray)
{ 
	switch (lMethodNum)
	{
	case eMethShowMsgBox:
	{
		if (eAppCapabilities1 <= g_capabilities)
		{
			IAddInDefBaseEx* cnn = (IAddInDefBaseEx*)m_iConnect;
			IMsgBox* imsgbox = (IMsgBox*)cnn->GetInterface(eIMsgBox);
			if (imsgbox)
			{
				IPlatformInfo* info = (IPlatformInfo*)cnn->GetInterface(eIPlatformInfo);
				assert(info);
				const IPlatformInfo::AppInfo* plt = info->GetPlatformInfo();
				if (!plt)
					break;
				tVariant retVal;
				tVarInit(&retVal);
				if (imsgbox->Confirm(plt->AppVersion, &retVal))
				{
					bool succeed = TV_BOOL(&retVal);
					WCHAR_T* result = 0;

					if (succeed)
						::convToShortWchar(&result, L"OK");
					else
						::convToShortWchar(&result, L"Cancel");

					imsgbox->Alert(result);
					delete[] result;

				}
			}
		}
		break;
	}
	case eSleep:
	{
		unsigned int sleep_time = (paParams)->ulVal;
		Sleep(sleep_time);
		break;
	}
	default:
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum,
                tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{ 
    bool ret = false;

    switch(lMethodNum)
    {
	case eMatch:
	{
		std::wstring data = std::wstring((wchar_t*)((paParams)->pwstrVal));
		std::wstring _regexp = std::wstring((wchar_t*)((paParams + 1)->pwstrVal));
		std::wregex _wregexp;
		try 
		{
			if (isCaseSensitive)
				_wregexp = std::wregex(_regexp);
			else
				_wregexp = std::wregex(_regexp, std::regex::icase);
		}
		catch (const std::regex_error& e) {
			//if (e.code() == std::regex_constants::error_brack) {			}
			addError(ADDIN_E_VERY_IMPORTANT, L"RegExp", L"Error in expression. Code: " + e.code(), e.code());
			ret = false;
			//TV_VT(pvarRetValue) = VTYPE_INT;
			//pvarRetValue->intVal = 0;
			break;
		}

		try
		{
			bool result = std::regex_match(data, _wregexp);
			TV_VT(pvarRetValue) = VTYPE_BOOL;
			pvarRetValue->bVal = result;
			ret = true;
			break;
		}
		catch (const std::regex_error& e)
		{
			addError(ADDIN_E_VERY_IMPORTANT, L"RegExp", L"Error while match execute. Code: " + e.code(), e.code());
			ret = false;
			//TV_VT(pvarRetValue) = VTYPE_INT;
			//pvarRetValue->intVal = 0;
			break;
		}
	}
	case eReplace:
	{
		std::wstring data_a = std::wstring((wchar_t*)((paParams)->pwstrVal));
		std::wstring data_b = std::wstring((wchar_t*)((paParams + 1)->pwstrVal));
		std::wstring _regexp = std::wstring((wchar_t*)((paParams + 2)->pwstrVal));
		std::wregex _wregexp;
		try
		{
			if (isCaseSensitive)
				_wregexp = std::wregex(_regexp);
			else
				_wregexp = std::wregex(_regexp, std::regex::icase);
		}
		catch (const std::regex_error& e) {
			//if (e.code() == std::regex_constants::error_brack) {			}
			addError(ADDIN_E_VERY_IMPORTANT, L"RegExp", L"Error in expression. Code: " + e.code(), e.code());
			ret = false;
			TV_VT(pvarRetValue) = VTYPE_INT;
			pvarRetValue->intVal = 0;
			break;
		}
		try
		{
			std::wstring result = std::regex_replace(data_a, _wregexp, data_b);
			wstring_to_p(result, pvarRetValue);
		}
		catch (const std::regex_error& e)
		{
			addError(ADDIN_E_VERY_IMPORTANT, L"RegExp", L"Error in statement. Code: " + e.code(), e.code());
			ret = false;
			//TV_VT(pvarRetValue) = VTYPE_INT;
			//pvarRetValue->intVal = 0;
			break;
		};
		
		ret = true;
		break;
	}
	case eSearch:
	{
		std::wstring data = std::wstring((wchar_t*)((paParams)->pwstrVal));
		std::wstring _regexp = std::wstring((wchar_t*)((paParams + 1)->pwstrVal));
		std::wregex _wregexp;
		try
		{
			if (isCaseSensitive)
				_wregexp = std::wregex(_regexp);
			else
				_wregexp = std::wregex(_regexp, std::regex::icase);
		}
		catch (const std::regex_error& e) {
			addError(ADDIN_E_VERY_IMPORTANT, L"RegExp", L"Error in expression. Code: " + e.code(), e.code());
			ret = false;
			TV_VT(pvarRetValue) = VTYPE_UI4;
			pvarRetValue->lVal = 0;
			break;
		}

		try
		{
			bool result = std::regex_search(data, mresult, _wregexp);

			TV_VT(pvarRetValue) = VTYPE_UI4;
			pvarRetValue->lVal = mresult.size();
			ret = true;
			break;
		}
		catch (const std::regex_error& e)
		{
			addError(ADDIN_E_VERY_IMPORTANT, L"RegExp", L"Error while match execute. Code: " + e.code(), e.code());
			ret = false;
			TV_VT(pvarRetValue) = VTYPE_UI4;
			pvarRetValue->lVal = 0;
			break;
		}

		break;
	}
	case eGetSearchResult:
	{
		unsigned int const index = ((paParams)->uintVal);
		
		if ((mresult).size() <= index)
		{
			addError(ADDIN_E_VERY_IMPORTANT, L"RegExp", L"Out of range", -1);
			ret = false;
		}

		std::sub_match<std::_String_const_iterator<std::_String_val<std::_Simple_types<wchar_t> > > > sm;
        sm = (mresult)[index];
        //sm = gsl::at(mresult, index);  
		TV_VT(pvarRetValue) = VTYPE_UI4;
		pvarRetValue->lVal = 0;
		ret = true;
		break;
	}
    case eClipboardCopy:
    {
        ret = true;

        TV_VT(pvarRetValue) = VTYPE_BOOL;
        pvarRetValue->bVal = false;

        if (!OpenClipboard(0))
        {
            ret = false;
            break;
        }

        EmptyClipboard();

        wchar_t* const copydata = (paParams)->pwstrVal;

 
        size_t const size = wcslen(copydata) * 2 + 4;
        if (size > 4)
        {
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size);
            memcpy(GlobalLock(hMem), copydata, size);
            GlobalUnlock(hMem);

            if (::SetClipboardData(CF_UNICODETEXT, hMem) == NULL)
                ret = false;
        }

        wchar_t* const copyHTMLdata = (paParams + 1)->pwstrVal;
        LPCWSTR lpszWide = (paParams + 1)->pwstrVal;

        int const nUtf8Size = ::WideCharToMultiByte(CP_UTF8, 0, lpszWide, -1, NULL, 0, NULL, NULL);
        if (nUtf8Size >= 1)
        {
            const int nDescLen = 105;
            HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, nDescLen + nUtf8Size);
            if (NULL != hGlobal)
            {
                bool bErr = false;
                LPSTR lpszBuf = static_cast<LPSTR>(::GlobalLock(hGlobal));
                LPSTR lpszUtf8 = lpszBuf + nDescLen;
                if (::WideCharToMultiByte(CP_UTF8, 0, lpszWide, -1, lpszUtf8, nUtf8Size, NULL, NULL) <= 0)
                {
                    bErr = true;
                }
                else
                {
                    LPCSTR lpszStartFrag = strstr(lpszUtf8, "<!--StartFragment-->");
                    LPCSTR lpszEndFrag = strstr(lpszUtf8, "<!--EndFragment-->");
                    lpszStartFrag += strlen("<!--StartFragment-->") + 2;

                    int i = _snprintf(
                        lpszBuf, nDescLen,
                        "Version:1.0\r\nStartHTML:%010d\r\nEndHTML:%010d\r\nStartFragment:%010d\r\nEndFragment:%010d\r\n",
                        nDescLen,
                        nDescLen + nUtf8Size - 1,        // offset to next char behind string
                        nDescLen + static_cast<int>(lpszStartFrag - lpszUtf8),
                        nDescLen + static_cast<int>(lpszEndFrag - lpszUtf8));
                }
                ::GlobalUnlock(hGlobal);
                if (bErr)
                {
                    ::GlobalFree(hGlobal);
                    hGlobal = NULL;
                }

                // Get clipboard id for HTML format...
                static int cfid = 0;
                cfid = RegisterClipboardFormat(L"HTML Format");
                // Open the clipboard...

                HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, strlen(lpszBuf) + 4);
                char* ptr = (char*)GlobalLock(hText);
                strcpy(ptr, lpszBuf);
                GlobalUnlock(hText);
                ::SetClipboardData(cfid, hText);
                GlobalFree(hText);
                
            }
        }

        CloseClipboard();

        TV_VT(pvarRetValue) = VTYPE_BOOL;
        pvarRetValue->bVal = true;

        break;
    }
    }

    return ret; 
}



//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
#if !defined( __linux__ ) && !defined(__APPLE__)
    _wsetlocale(LC_ALL, loc);
#else
    //We convert in char* char_locale
    //also we establish locale
    //setlocale(LC_ALL, char_locale);
#endif
}
/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem)
{
    m_iMemory = (IMemoryManager*)mem;
    return m_iMemory != 0;
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source, 
                        const wchar_t* descriptor, long code)
{
    if (m_iConnect)
    {
        WCHAR_T *err = 0;
        WCHAR_T *descr = 0;
        
        ::convToShortWchar(&err, source);
        ::convToShortWchar(&descr, descriptor);

        m_iConnect->AddError(wcode, err, descr, code);
        delete[] err;
        delete[] descr;
    }
}
//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name, 
                        const uint32_t size) const
{
    long ret = -1;
    for (uint32_t i = 0; i < size; i++)
    {
        if (!wcscmp(names[i], name))
        {
            ret = i;
            break;
        }
    }
    return ret;
}
//---------------------------------------------------------------------------//
char* CAddInNative::wstr_to_char_p(std::wstring wStr)
{
	const wchar_t *str = wStr.c_str();
	size_t size = wcslen(str) * 2 + 2;
	char * StartPoint = new char[size];
	size_t c_size;
	wcstombs_s(&c_size, StartPoint, size, str, size);

	return StartPoint;
}
//---------------------------------------------------------------------------//
bool CAddInNative::wstring_to_p(std::wstring str, tVariant* val) 
{
	char* t1;
	TV_VT(val) = VTYPE_PWSTR;
	m_iMemory->AllocMemory((void**)&t1, (str.length() + 1) * sizeof(WCHAR_T));
	memcpy(t1, str.c_str(), (str.length() + 1) * sizeof(WCHAR_T));
	val->pstrVal = t1;
	val->strLen = str.length();
	return true;
}
//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
    if (!len)
        len = ::wcslen(Source) + 1;

    if (!*Dest)
        *Dest = new WCHAR_T[len];

    WCHAR_T* tmpShort = *Dest;
    wchar_t* tmpWChar = (wchar_t*) Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len * sizeof(WCHAR_T));
#ifdef __linux__
    size_t succeed = (size_t)-1;
    size_t f = len * sizeof(wchar_t), t = len * sizeof(WCHAR_T);
    const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
    iconv_t cd = iconv_open("UTF-16LE", fromCode);
    if (cd != (iconv_t)-1)
    {
        succeed = iconv(cd, (char**)&tmpWChar, &f, (char**)&tmpShort, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return (uint32_t)succeed;
    }
#endif //__linux__
    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpShort = (WCHAR_T)*tmpWChar;
    }

    return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
    if (!len)
        len = getLenShortWcharStr(Source) + 1;

    if (!*Dest)
        *Dest = new wchar_t[len];

    wchar_t* tmpWChar = *Dest;
    WCHAR_T* tmpShort = (WCHAR_T*)Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len * sizeof(wchar_t));
#ifdef __linux__
    size_t succeed = (size_t)-1;
    const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
    size_t f = len * sizeof(WCHAR_T), t = len * sizeof(wchar_t);
    iconv_t cd = iconv_open("UTF-32LE", fromCode);
    if (cd != (iconv_t)-1)
    {
        succeed = iconv(cd, (char**)&tmpShort, &f, (char**)&tmpWChar, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return (uint32_t)succeed;
    }
#endif //__linux__
    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpWChar = (wchar_t)*tmpShort;
    }

    return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
    uint32_t res = 0;
    WCHAR_T *tmpShort = (WCHAR_T*)Source;

    while (*tmpShort++)
        ++res;

    return res;
}
//---------------------------------------------------------------------------//
#ifdef LINUX_OR_MACOS
WcharWrapper::WcharWrapper(const WCHAR_T* str) : m_str_WCHAR(NULL),
                           m_str_wchar(NULL)
{
    if (str)
    {
        int len = getLenShortWcharStr(str);
        m_str_WCHAR = new WCHAR_T[len + 1];
        memset(m_str_WCHAR,   0, sizeof(WCHAR_T) * (len + 1));
        memcpy(m_str_WCHAR, str, sizeof(WCHAR_T) * len);
        ::convFromShortWchar(&m_str_wchar, m_str_WCHAR);
    }
}
#endif
//---------------------------------------------------------------------------//
WcharWrapper::WcharWrapper(const wchar_t* str) :
#ifdef LINUX_OR_MACOS
    m_str_WCHAR(NULL),
#endif 
    m_str_wchar(NULL)
{
    if (str)
    {
        int len = wcslen(str);
        m_str_wchar = new wchar_t[len + 1];
        memset(m_str_wchar, 0, sizeof(wchar_t) * (len + 1));
        memcpy(m_str_wchar, str, sizeof(wchar_t) * len);
#ifdef LINUX_OR_MACOS
        ::convToShortWchar(&m_str_WCHAR, m_str_wchar);
#endif
    }

}
//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
#ifdef LINUX_OR_MACOS
    if (m_str_WCHAR)
    {
        delete [] m_str_WCHAR;
        m_str_WCHAR = NULL;
    }
#endif
    if (m_str_wchar)
    {
        delete [] m_str_wchar;
        m_str_wchar = NULL;
    }
}
//---------------------------------------------------------------------------//
