#pragma once

#include <windows.h>
#include <ctime>
#include <cassert>
#include <string>

// #define USE_GLOBAL_TIME for globalTime (UTC, GMT) else localTime

class CTimeSpan {
    __time64_t m_timeSpan;
public:
    CTimeSpan() throw(): m_timeSpan(0) {}
    explicit CTimeSpan(__time64_t time) throw(): m_timeSpan(time) {}
    CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs) throw();

    LONGLONG GetDays() const throw();
    LONGLONG GetTotalHours() const throw();
    LONG GetHours() const throw();
    LONGLONG GetTotalMinutes() const throw();
    LONG GetMinutes() const throw();
    LONGLONG GetTotalSeconds() const throw();
    LONG GetSeconds() const throw();

    __time64_t GetTimeSpan() const throw() { return m_timeSpan; }

    CTimeSpan operator+(CTimeSpan span) const throw();
    CTimeSpan operator-(CTimeSpan span) const throw();
    CTimeSpan& operator+=(CTimeSpan span) throw();
    CTimeSpan& operator-=(CTimeSpan span) throw();
    bool operator==(CTimeSpan span) const throw();
    bool operator!=(CTimeSpan span) const throw();
    bool operator<(CTimeSpan span) const throw();
    bool operator>(CTimeSpan span) const throw();
    bool operator<=(CTimeSpan span) const throw();
    bool operator>=(CTimeSpan span) const throw();
};

class CTime {
    __time64_t m_time;
public:
    CTime() throw(): m_time(0) {}
    explicit CTime(__time64_t time) throw(): m_time(time) {}
    CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = 0);

    bool GetTm(struct tm* ptm) const;

    CTime& operator=(__time64_t time) throw();

    CTime& operator+=(CTimeSpan span) throw();
    CTime& operator-=(CTimeSpan span) throw();

    CTimeSpan operator-(CTime time) const throw();
    CTime operator-(CTimeSpan span) const throw();
    CTime operator+(CTimeSpan span) const throw();

    bool operator==(CTime time) const throw();
    bool operator!=(CTime time) const throw();
    bool operator<(CTime time) const throw();
    bool operator>(CTime time) const throw();
    bool operator<=(CTime time) const throw();
    bool operator>=(CTime time) const throw();

    __time64_t GetTime() const throw() { return m_time; }

    int GetYear() const throw();
    int GetMonth() const throw();
    int GetDay() const throw();
    int GetHour() const throw();
    int GetMinute() const throw();
    int GetSecond() const throw();
    int GetDayOfWeek() const throw();

    std::string Format(const char* format) const;
    std::wstring Format(const wchar_t* format) const;

    static CTime GetCurrentTime() throw() { return {CTime(::_time64(nullptr))}; }
};

/////////////////////////////////////////////////////////////////////////////
// CTimeSpan
/////////////////////////////////////////////////////////////////////////////

inline CTimeSpan::CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs) throw() {
    m_timeSpan = nSecs + 60 * (nMins + 60 * (nHours + __int64(24) * lDays));
}

inline LONGLONG CTimeSpan::GetDays() const throw() {
    return (m_timeSpan / (24 * 3600));
}

inline LONGLONG CTimeSpan::GetTotalHours() const throw() {
    return (m_timeSpan / 3600);
}

inline LONG CTimeSpan::GetHours() const throw() {
    return (LONG(GetTotalHours() - (GetDays() * 24)));
}

inline LONGLONG CTimeSpan::GetTotalMinutes() const throw() {
    return (m_timeSpan / 60);
}

inline LONG CTimeSpan::GetMinutes() const throw() {
    return (LONG(GetTotalMinutes() - (GetTotalHours() * 60)));
}

inline LONGLONG CTimeSpan::GetTotalSeconds() const throw() {
    return m_timeSpan;
}

inline LONG CTimeSpan::GetSeconds() const throw() {
    return (LONG(GetTotalSeconds() - (GetTotalMinutes() * 60)));
}

inline CTimeSpan CTimeSpan::operator+(CTimeSpan span) const throw() {
    return (CTimeSpan(m_timeSpan + span.m_timeSpan));
}

inline CTimeSpan CTimeSpan::operator-(CTimeSpan span) const throw() {
    return (CTimeSpan(m_timeSpan - span.m_timeSpan));
}

inline CTimeSpan& CTimeSpan::operator+=(CTimeSpan span) throw() {
    m_timeSpan += span.m_timeSpan;
    return (*this);
}

inline CTimeSpan& CTimeSpan::operator-=(CTimeSpan span) throw() {
    m_timeSpan -= span.m_timeSpan;
    return (*this);
}

inline bool CTimeSpan::operator==(CTimeSpan span) const throw() {
    return (m_timeSpan == span.m_timeSpan);
}

inline bool CTimeSpan::operator!=(CTimeSpan span) const throw() {
    return (m_timeSpan != span.m_timeSpan);
}

inline bool CTimeSpan::operator<(CTimeSpan span) const throw() {
    return (m_timeSpan < span.m_timeSpan);
}

inline bool CTimeSpan::operator>(CTimeSpan span) const throw() {
    return (m_timeSpan > span.m_timeSpan);
}

inline bool CTimeSpan::operator<=(CTimeSpan span) const throw() {
    return (m_timeSpan <= span.m_timeSpan);
}

inline bool CTimeSpan::operator>=(CTimeSpan span) const throw() {
    return (m_timeSpan >= span.m_timeSpan);
}

/////////////////////////////////////////////////////////////////////////////
// CTime
/////////////////////////////////////////////////////////////////////////////

inline CTime::CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST) {
    assert(nYear >= 1900);
    assert(nMonth >= 1 && nMonth <= 12);
    assert(nDay >= 1 && nDay <= 31);
    assert(nHour >= 0 && nHour <= 23);
    assert(nMin >= 0 && nMin <= 59);
    assert(nSec >= 0 && nSec <= 59);

    struct tm t;
    t.tm_sec = nSec;
    t.tm_min = nMin;
    t.tm_hour = nHour;
    t.tm_mday = nDay;
    t.tm_mon = nMonth - 1;        // tm_mon is 0 based
    t.tm_year = nYear - 1900;     // tm_year is 1900 based
    t.tm_isdst = nDST;
#ifdef    USE_GLOBAL_TIME
    m_time = _mkgmtime64(&t);
#else
    m_time = _mktime64(&t);
#endif
    assert(m_time != -1);       // indicates an illegal input time
    if( m_time == -1 )
        throw -1; // E_INVALIDARG;
}

inline bool CTime::GetTm(struct tm* ptm) const {
    if( ptm == nullptr ) return false;
#ifdef    USE_GLOBAL_TIME
    return _gmtime64_s(ptm,&m_time)==0;
#else
    return _localtime64_s(ptm, &m_time) == 0;
#endif
}

inline CTime& CTime::operator=(__time64_t time) throw() {
    m_time = time;
    return (*this);
}

inline CTime& CTime::operator+=(CTimeSpan span) throw() {
    m_time += span.GetTimeSpan();
    return (*this);
}

inline CTime& CTime::operator-=(CTimeSpan span) throw() {
    m_time -= span.GetTimeSpan();
    return (*this);
}

inline CTimeSpan CTime::operator-(CTime time) const throw() {
    return CTimeSpan(m_time - time.m_time);
}

inline CTime CTime::operator-(CTimeSpan span) const throw() {
    return CTime(m_time - span.GetTimeSpan());
}

inline CTime CTime::operator+(CTimeSpan span) const throw() {
    return CTime(m_time + span.GetTimeSpan());
}

inline bool CTime::operator==(CTime time) const throw() {
    return (m_time == time.m_time);
}

inline bool CTime::operator!=(CTime time) const throw() {
    return (m_time != time.m_time);
}

inline bool CTime::operator<(CTime time) const throw() {
    return (m_time < time.m_time);
}

inline bool CTime::operator>(CTime time) const throw() {
    return (m_time > time.m_time);
}

inline bool CTime::operator<=(CTime time) const throw() {
    return (m_time <= time.m_time);
}

inline bool CTime::operator>=(CTime time) const throw() {
    return (m_time >= time.m_time);
}

inline int CTime::GetYear() const throw() {
    struct tm t;
    return GetTm(&t) ? (t.tm_year) + 1900 : 0;
}

inline int CTime::GetMonth() const throw() {
    struct tm t;
    return GetTm(&t) ? t.tm_mon + 1 : 0;
}

inline int CTime::GetDay() const throw() {
    struct tm t;
    return GetTm(&t) ? t.tm_mday : 0;
}

inline int CTime::GetHour() const throw() {
    struct tm t;
    return GetTm(&t) ? t.tm_hour : -1;
}

inline int CTime::GetMinute() const throw() {
    struct tm t;
    return GetTm(&t) ? t.tm_min : -1;
}

inline int CTime::GetSecond() const throw() {
    struct tm t;
    return GetTm(&t) ? t.tm_sec : -1;
}

inline int CTime::GetDayOfWeek() const throw() {
    struct tm t;
    return GetTm(&t) ? t.tm_wday + 1 : 0;
}

inline std::string CTime::Format(const char* format) const {
    struct tm t;
    if( !GetTm(&t) )
        return "";
    char buf[80];
    if( strftime(buf, 80, format, &t) > 0 )
        return buf;
    return "";
}

inline std::wstring CTime::Format(const wchar_t* format) const {
    struct tm t;
    if( !GetTm(&t) )
        return L"";
    wchar_t buf[80];
    if( wcsftime(buf, 80, format, &t) > 0 )
        return buf;
    return L"";
}

class CTick {
private:
    clock_t dw{};
public:
    CTick() { start(); }
    void start() { dw = clock(); }
    void stop() { dw = clock() - dw; }
    DWORD get() const { return static_cast<DWORD>(((static_cast<double>(dw)) / CLOCKS_PER_SEC) * 1000.0); }
};
