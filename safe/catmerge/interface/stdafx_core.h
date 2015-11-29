#ifndef __stdafx_core_h
#define __stdafx_core_h
/*=======================*/

/* this replaces the core contents of stdafx files in the non-Windows use case */

/* all of this fine "stuff" should eventually be eliminated from use in the code base as it provides no real value */

/*=======================*/

#include "windows.h"
//include <cstring.h>
#include <assert.h>
#include "csx_ext.h"

#define __AFXWIN_H__

/*=======================*/

#define DECLARE_MESSAGE_MAP()
#define END_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(foo, bar)

/*=======================*/

#define STDMETHOD(method)       HRESULT  method
#define STDMETHODCALLTYPE
#define STDMETHODIMP            HRESULT STDMETHODCALLTYPE

/*=======================*/

#define _ASSERT(expr) assert(expr)

/*=======================*/

CSX_STATIC_INLINE VOID
TRACE(
    LPCTSTR lpszFormat,
    ...)
{
    va_list ap;
    va_start(ap, lpszFormat);
    csx_p_dump_native_stringv(lpszFormat, ap);
    va_end(ap);
}

/*=======================*/
#if 0
#define STDAFX_NOTE_MFC_USAGE() \
    CsxStubNoteMfcUsage(__FILE__, __LINE__, __func__)
#endif
#define STDAFX_NOTE_MFC_USAGE()

/*=======================*/

class CWinApp {
  public:
    static int ExitInstance(void) { CSX_PANIC(); return 0; }
};

/*=======================*/

class CObject {
  public:
    CObject() { STDAFX_NOTE_MFC_USAGE(); }
    ~CObject() { STDAFX_NOTE_MFC_USAGE(); }
};

class CObArray {
  public:
    CObArray() { STDAFX_NOTE_MFC_USAGE(); }
    ~CObArray() { STDAFX_NOTE_MFC_USAGE(); }
    INT_PTR GetSize() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    INT_PTR Add(CObject * newElement) { STDAFX_NOTE_MFC_USAGE(); return 0; }
    void RemoveAll() { STDAFX_NOTE_MFC_USAGE(); }
    CObject * GetAt(INT_PTR nIndex) const { STDAFX_NOTE_MFC_USAGE(); return NULL; }
};

class CByteArray: public CObject {
  public:
    CByteArray() { STDAFX_NOTE_MFC_USAGE(); }
    ~CByteArray() { STDAFX_NOTE_MFC_USAGE(); }
    INT_PTR GetSize() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    INT_PTR Add(BYTE newElement) { STDAFX_NOTE_MFC_USAGE(); return 0; }
    void RemoveAll() { STDAFX_NOTE_MFC_USAGE(); }
    BYTE GetAt(INT_PTR nIndex) const { STDAFX_NOTE_MFC_USAGE(); return 0; }
};

/*=======================*/

class CTimeSpan {
  public:
    CTimeSpan() { STDAFX_NOTE_MFC_USAGE(); }
    CTimeSpan(DWORD &) { STDAFX_NOTE_MFC_USAGE(); }
    ~CTimeSpan() { STDAFX_NOTE_MFC_USAGE(); }
};

class CTime {
  static CTime tmp1;
  static CTimeSpan tmp3;
  public:
    CTime() { STDAFX_NOTE_MFC_USAGE(); }
    ~CTime() { STDAFX_NOTE_MFC_USAGE(); }
    static CTime GetCurrentTime() { STDAFX_NOTE_MFC_USAGE(); return tmp1; }
    int GetYear() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    int GetMonth() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    int GetDay() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    int GetHour() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    int GetMinute() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    int GetSecond() const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    //CStdString Format(LPCTSTR pFormat) const { STDAFX_NOTE_MFC_USAGE(); return "whatever dude"; }
    CTimeSpan operator-(CTime time) const { STDAFX_NOTE_MFC_USAGE(); return tmp3; }
    CTime operator-(CTimeSpan timeSpan) const { STDAFX_NOTE_MFC_USAGE(); return tmp1; }
    CTime operator+(CTimeSpan timeSpan) const { STDAFX_NOTE_MFC_USAGE(); return tmp1; }
    const CTime & operator+=(CTimeSpan timeSpan) { STDAFX_NOTE_MFC_USAGE(); return tmp1; }
    const CTime & operator-=(CTimeSpan timeSpan) { STDAFX_NOTE_MFC_USAGE(); return tmp1; }
    BOOL operator<=(CTime time) const { STDAFX_NOTE_MFC_USAGE(); return 0; }
    BOOL operator>=(CTime time) const { STDAFX_NOTE_MFC_USAGE(); return 0; }
};

/*=======================*/

class CFileException {
  public:
    CFileException() { STDAFX_NOTE_MFC_USAGE(); }
    ~CFileException() { STDAFX_NOTE_MFC_USAGE(); }
    void Delete() { STDAFX_NOTE_MFC_USAGE(); }
    LONG m_lOsError;
};

/*=======================*/
#endif /* __stdafx_core_h */
