// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/crown-config.h>
#endif

#include <util/time.h>

#include <util/check.h>

#include <atomic>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ctime>
#include <thread>

#include <tinyformat.h>

void UninterruptibleSleep(const std::chrono::microseconds& n) { std::this_thread::sleep_for(n); }

static std::atomic<int64_t> nMockTime(0); //!< For testing

int64_t GetTime()
{
    int64_t mocktime = nMockTime.load(std::memory_order_relaxed);
    if (mocktime) return mocktime;

    time_t now = time(nullptr);
    assert(now > 0);
    return now;
}

template <typename T>
T GetTime()
{
    const std::chrono::seconds mocktime{nMockTime.load(std::memory_order_relaxed)};

    return std::chrono::duration_cast<T>(
        mocktime.count() ?
            mocktime :
            std::chrono::microseconds{GetTimeMicros()});
}
template std::chrono::seconds GetTime();
template std::chrono::milliseconds GetTime();
template std::chrono::microseconds GetTime();

void SetMockTime(int64_t nMockTimeIn)
{
    Assert(nMockTimeIn >= 0);
    nMockTime.store(nMockTimeIn, std::memory_order_relaxed);
}

int64_t GetMockTime()
{
    return nMockTime.load(std::memory_order_relaxed);
}

int64_t GetTimeMillis()
{
    int64_t now = (boost::posix_time::microsec_clock::universal_time() -
                   boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_milliseconds();
    assert(now > 0);
    return now;
}

int64_t GetTimeMicros()
{
    int64_t now = (boost::posix_time::microsec_clock::universal_time() -
                   boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_microseconds();
    assert(now > 0);
    return now;
}

int64_t GetSystemTimeInSeconds()
{
    return GetTimeMicros()/1000000;
}

std::string FormatISO8601DateTime(int64_t nTime) {
    struct tm ts;
    time_t time_val = nTime;
#ifdef HAVE_GMTIME_R
    if (gmtime_r(&time_val, &ts) == nullptr) {
#else
    if (gmtime_s(&ts, &time_val) != 0) {
#endif
        return {};
    }
    return strprintf("%04i-%02i-%02iT%02i:%02i:%02iZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}

std::string FormatISO8601Date(int64_t nTime) {
    struct tm ts;
    time_t time_val = nTime;
#ifdef HAVE_GMTIME_R
    if (gmtime_r(&time_val, &ts) == nullptr) {
#else
    if (gmtime_s(&ts, &time_val) != 0) {
#endif
        return {};
    }
    return strprintf("%04i-%02i-%02i", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday);
}

int64_t ParseISO8601DateTime(const std::string& str)
{
    static const boost::posix_time::ptime epoch = boost::posix_time::from_time_t(0);
    static const std::locale loc(std::locale::classic(),
        new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%SZ"));
    std::istringstream iss(str);
    iss.imbue(loc);
    boost::posix_time::ptime ptime(boost::date_time::not_a_date_time);
    iss >> ptime;
    if (ptime.is_not_a_date_time() || epoch > ptime)
        return 0;
    return (ptime - epoch).total_seconds();
}

std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime)
{
    static std::locale classic(std::locale::classic());
    // std::locale takes ownership of the pointer
    std::locale loc(classic, new boost::posix_time::time_facet(pszFormat));
    std::stringstream ss;
    ss.imbue(loc);
    ss << boost::posix_time::from_time_t(nTime);
    return ss.str();
}

std::string DurationToDHMS(int64_t nDurationTime)
{
    int seconds = nDurationTime % 60;
    nDurationTime /= 60;
    int minutes = nDurationTime % 60;
    nDurationTime /= 60;
    int hours = nDurationTime % 24;
    int days = nDurationTime / 24;
    if(days)
        return strprintf("%dd %02dh:%02dm:%02ds", days, hours, minutes, seconds);
    if(hours)
        return strprintf("%02dh:%02dm:%02ds", hours, minutes, seconds);
    return strprintf("%02dm:%02ds", minutes, seconds);
}

namespace part
{
std::string GetTimeString(int64_t timestamp, char *buffer, size_t nBuffer)
{
    struct tm* dt;
    time_t t = timestamp;
    dt = localtime(&t);

    strftime(buffer, nBuffer, "%Y-%m-%dT%H:%M:%S%z", dt); // %Z shows long strings on windows
    return std::string(buffer); // copies the null-terminated character sequence
}

static int daysInMonth(int year, int month)
{
    return month == 2 ? (year % 4 ? 28 : (year % 100 ? 29 : (year % 400 ? 28 : 29))) : ((month - 1) % 7 % 2 ? 30 : 31);
}

int64_t strToEpoch(const char *input, bool fFillMax)
{
    int year, month, day, hours, minutes, seconds;
    int n = sscanf(input, "%d-%d-%dT%d:%d:%d",
        &year, &month, &day, &hours, &minutes, &seconds);

    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    if (n > 0 && year >= 1970 && year <= 9999)
        tm.tm_year = year - 1900;
    if (n > 1 && month > 0 && month < 13)
        tm.tm_mon = month - 1;          else if (fFillMax) { tm.tm_mon = 11; month = 12; }
    if (n > 2 && day > 0 && day < 32)
        tm.tm_mday = day;               else tm.tm_mday = fFillMax ? daysInMonth(year, month) : 1;
    if (n > 3 && hours >= 0 && hours < 24)
        tm.tm_hour = hours;             else if (fFillMax) tm.tm_hour = 23;
    if (n > 4 && minutes >= 0 && minutes < 60)
        tm.tm_min = minutes;            else if (fFillMax) tm.tm_min = 59;
    if (n > 5 && seconds >= 0 && seconds < 60)
        tm.tm_sec = seconds;            else if (fFillMax) tm.tm_sec = 59;

    return (int64_t) mktime(&tm);
}
}
