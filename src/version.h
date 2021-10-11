// Copyright (c) 2012-2018 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_VERSION_H
#define CROWN_VERSION_H

/**
 * network protocol versioning
 */

static const int PROTOCOL_VERSION = 70058;

static const int PROTOCOL_POS_START = 70057;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = PROTOCOL_VERSION;

//! minimum proto version of masternode to accept in DKGs
static const int MIN_MASTERNODE_PROTO_VERSION = 70057;
//! minimum peer version accepted by legacySigner
static const int MIN_POOL_PEER_PROTO_VERSION = 70057;

//! minimum peer version for masternode budgets
static const int MIN_BUDGET_PEER_PROTO_VERSION = 70057;

//! minimum peer version for masternode winner broadcasts
static const int MIN_MNW_PEER_PROTO_VERSION = 70057;

//! minimum version to get version 2 masternode ping messages
static const int MIN_MNW_PING_VERSION = 70057;

//! minimum peer version that can receive masternode payments
// V1 - Last protocol version before update
// V2 - Newest protocol version
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_PREV = 70057;
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_CURR = 70058;

//! minimum peer version that can receive systemnode payments
// V1 - Last protocol version before update
// V2 - Newest protocol version
static const int MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION_PREV = 70057;
static const int MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION_CURR = 70058;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 70011;

//! "sendheaders" command and announcing blocks with headers starts with this version
static const int SENDHEADERS_VERSION = 70012;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
static const int FEEFILTER_VERSION = 70013;

//! short-id-based block download starts with this version
static const int SHORT_IDS_BLOCKS_VERSION = 70014;

//! not banning for invalid compact blocks starts with this version
static const int INVALID_CB_NO_BAN_VERSION = 70015;

//! "wtxidrelay" command for wtxid-based relay starts with this version
static const int WTXID_RELAY_VERSION = 70016;

// Make sure that none of the values above collide with
// `SERIALIZE_TRANSACTION_NO_WITNESS` or `ADDRV2_FORMAT`.

#endif // CROWN_VERSION_H
