// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_NODESYNC_H
#define CROWN_NODESYNC_H

#include <net.h>

class CConnman;

std::string currentSyncStatus();
void ThreadNodeSync(CConnman& connman);

#endif // CROWN_NODESYNC_H
