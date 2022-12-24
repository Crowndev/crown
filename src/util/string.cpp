// Copyright (c) 2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>
#include <algorithm>
#include <cstring>
#include <tinyformat.h>

namespace part
{
static bool icompare_pred(unsigned char a, unsigned char b)
{
    return std::tolower(a) == std::tolower(b);
};
static bool icompare_str(const std::string &a, const std::string &b)
{
    return a.length() == b.length()
        && std::equal(b.begin(), b.end(), a.begin(), icompare_pred);
};

void *memrchr(const void *s, int c, size_t n)
{
    if (n < 1)
        return nullptr;

    unsigned char *cp = (unsigned char*) s + n;

    do {
        if (*(--cp) == (unsigned char) c)
            return (void*) cp;
    } while (--n != 0);

    return nullptr;
};

// memcmp_nta - memcmp that is secure against timing attacks
// returns 0 if both areas are equal to each other, non-zero otherwise
int memcmp_nta(const void *cs, const void *ct, size_t count)
{
    const unsigned char *su1, *su2;
    int res = 0;

    for (su1 = (unsigned char*)cs, su2 = (unsigned char*)ct;
        0 < count; ++su1, ++su2, count--)
        res |= (*su1 ^ *su2);

    return res;
};

void ReplaceStrInPlace(std::string &subject, const std::string search, const std::string replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
};

bool IsStringBoolPositive(const std::string &value)
{
    return (value == "+" || value == "1" || icompare_str(value, "on")  || icompare_str(value, "true") || icompare_str(value, "yes") || icompare_str(value, "y"));
};

bool IsStringBoolNegative(const std::string &value)
{
    return (value == "-" || value == "0" || icompare_str(value, "off") || icompare_str(value, "false") || icompare_str(value, "no") || icompare_str(value, "n"));
};

bool GetStringBool(const std::string &value, bool &fOut)
{
    if (IsStringBoolPositive(value)) {
        fOut = true;
        return true;
    }
    if (IsStringBoolNegative(value)) {
        fOut = false;
        return true;
    }
    return false;
};

bool IsStrOnlyDigits(const std::string &s)
{
    return s.find_first_not_of("0123456789") == std::string::npos;
};

std::string BytesReadable(uint64_t nBytes)
{
    if (nBytes >= 1024ll*1024ll*1024ll*1024ll)
        return strprintf("%.2f TB", nBytes/1024.0/1024.0/1024.0/1024.0);
    if (nBytes >= 1024*1024*1024)
        return strprintf("%.2f GB", nBytes/1024.0/1024.0/1024.0);
    if (nBytes >= 1024*1024)
        return strprintf("%.2f MB", nBytes/1024.0/1024.0);
    if (nBytes >= 1024)
        return strprintf("%.2f KB", nBytes/1024.0);

    return strprintf("%d B", nBytes);
};

bool stringsMatchI(const std::string &sString, const std::string &sFind, int type)
{
    // case insensitive

    switch (type) {
        case 0: // full match
            return sString.length() == sFind.length()
                && std::equal(sFind.begin(), sFind.end(), sString.begin(), icompare_pred);
        case 1: // startswith
            return sString.length() >= sFind.length()
                && std::equal(sFind.begin(), sFind.end(), sString.begin(), icompare_pred);
        case 2: // endswith
            return sString.length() >= sFind.length()
                && std::equal(sFind.begin(), sFind.end(), sString.begin(), icompare_pred);
        case 3: // contains
            return std::search(sString.begin(), sString.end(), sFind.begin(), sFind.end(), icompare_pred) != sString.end();
    }

    return 0; // unknown type
};

std::string StripQuotes(std::string s)
{
    return TrimQuotes(s);
};

std::string &TrimQuotes(std::string &s)
{
    if (s.size() < 1)
        return s;
    if (s.front() == '"')
        s.erase(0, 1);

    size_t n = s.size();
    if (n < 1)
        return s;
    if (n > 1 && s[n-2] == '\\') // don't strip \"
        return s;
    if (s.back() == '"')
        s.erase(n - 1);
    return s;
};

std::string &TrimWhitespace(std::string &s)
{
    LTrimWhitespace(s);
    RTrimWhitespace(s);
    return s;
};

std::string &LTrimWhitespace(std::string &s)
{
    std::string::iterator i;
    for (i = s.begin(); i != s.end(); ++i)
        if (!std::isspace(*i))
            break;
    if (i != s.begin())
        s.erase(s.begin(), i);
    return s;
};

std::string &RTrimWhitespace(std::string &s)
{
    std::string::reverse_iterator i;
    for (i = s.rbegin(); i != s.rend(); ++i)
        if (!std::isspace(*i))
            break;
    if (i != s.rbegin())
        s.erase(i.base(), s.end());
    return s;
};

bool endsWith(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
};

} // namespace part

bool iequals(const std::string& str1, const std::string& str2)
{
    //return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(), [](char a, char b) {return tolower(a) == tolower(b);});
    return str1.size() == str2.size() && std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(), [](auto a, auto b){return std::tolower(a)==std::tolower(b);});

}
