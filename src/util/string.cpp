// Copyright (c) 2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

bool iequals(const std::string& str1, const std::string& str2)
{
    //return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(), [](char a, char b) {return tolower(a) == tolower(b);});
    return str1.size() == str2.size() && std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(), [](auto a, auto b){return std::tolower(a)==std::tolower(b);});

}
