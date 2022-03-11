// Copyright (c) 2014-2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_NODE_INIT_H
#define CROWN_NODE_INIT_H

#include <crown/instantx.h>
#include <crown/spork.h>
#include <masternode/activemasternode.h>
#include <masternode/masternode-budget.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternodeconfig.h>
#include <masternode/masternodeman.h>
#include <netbase.h>
#include <node/ui_interface.h>
#include <systemnode/activesystemnode.h>
#include <systemnode/systemnode-payments.h>
#include <systemnode/systemnodeconfig.h>
#include <systemnode/systemnodeman.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>

void loadNodeConfiguration();
bool setupNodeConfiguration();

#endif // CROWN_INIT_H
