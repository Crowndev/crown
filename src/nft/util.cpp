// Copyright (c) 2021 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <nft/util.h>

std::string convertToUpper(std::string inputString)
{
    std::string upperString;
    char const *ptr = inputString.c_str();
    for (unsigned int i = 0; i < upperString.size(); i++) {
       upperString += toupper(ptr[i]);
    }
    return upperString;
}
