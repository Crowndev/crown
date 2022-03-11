// Copyright (c) 2014-2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_CACHE_H
#define CROWN_CACHE_H

#include <flat-database.h>
#include <masternode/masternode-budget.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode.h>
#include <masternode/masternodeman.h>
#include <netfulfilledman.h>
#include <node/ui_interface.h>
#include <systemnode/systemnode-payments.h>
#include <systemnode/systemnode.h>
#include <systemnode/systemnodeman.h>
#include <util/system.h>
#include <util/translation.h>

void DumpCaches();
bool LoadCaches();

#endif // CROWN_CACHE_H
