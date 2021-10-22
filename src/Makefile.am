# Copyright (c) 2013-2016 The Crown Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%:
	@echo '$*' = '$($*)'

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

CROWN_INCLUDES=-I$(builddir) -I$(srcdir)/secp256k1/include $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS)

CROWN_INCLUDES += $(UNIVALUE_CFLAGS)

LIBCROWN_SERVER=libcrown_server.a
LIBCROWN_COMMON=libcrown_common.a
LIBCROWN_CONSENSUS=libcrown_consensus.a
LIBCROWN_CLI=libcrown_cli.a
LIBCROWN_UTIL=libcrown_util.a
LIBCROWN_CRYPTO_BASE=crypto/libcrown_crypto_base.a
LIBCROWNQT=qt/libcrownqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBCROWN_ZMQ=libcrown_zmq.a
endif
if BUILD_CROWN_LIBS
LIBCROWNCONSENSUS=libcrownconsensus.la
endif
if ENABLE_WALLET
LIBCROWN_WALLET=libcrown_wallet.a
LIBCROWN_WALLET_TOOL=libcrown_wallet_tool.a
endif

LIBCROWN_CRYPTO= $(LIBCROWN_CRYPTO_BASE)
if ENABLE_SSE41
LIBCROWN_CRYPTO_SSE41 = crypto/libcrown_crypto_sse41.a
LIBCROWN_CRYPTO += $(LIBCROWN_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBCROWN_CRYPTO_AVX2 = crypto/libcrown_crypto_avx2.a
LIBCROWN_CRYPTO += $(LIBCROWN_CRYPTO_AVX2)
endif
if ENABLE_SHANI
LIBCROWN_CRYPTO_SHANI = crypto/libcrown_crypto_shani.a
LIBCROWN_CRYPTO += $(LIBCROWN_CRYPTO_SHANI)
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBCROWN_CRYPTO) \
  $(LIBCROWN_UTIL) \
  $(LIBCROWN_COMMON) \
  $(LIBCROWN_CONSENSUS) \
  $(LIBCROWN_SERVER) \
  $(LIBCROWN_CLI) \
  $(LIBCROWN_WALLET) \
  $(LIBCROWN_WALLET_TOOL) \
  $(LIBCROWN_ZMQ)

lib_LTLIBRARIES = $(LIBCROWNCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_CROWND
  bin_PROGRAMS += crownd
endif

if BUILD_CROWN_NODE
  bin_PROGRAMS += crown-node
endif

if BUILD_CROWN_CLI
  bin_PROGRAMS += crown-cli
endif
if BUILD_CROWN_TX
  bin_PROGRAMS += crown-tx
endif
if ENABLE_WALLET
if BUILD_CROWN_WALLET
  bin_PROGRAMS += crown-wallet
endif
endif

.PHONY: FORCE check-symbols check-security
# crown core #
CROWN_CORE_H = \
  addrdb.h \
  addrman.h \
  attributes.h \
  auxpow.h \
  banman.h \
  base58.h \
  bech32.h \
  blockencodings.h \
  blockfilter.h \
  bloom.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/cpuid.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_check.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  crown/cache.h \
  crown/init.h \
  crown/instantx.h \
  crown/legacycalls.h \
  crown/legacysigner.h \
  crown/nodesync.h \
  crown/nodewallet.h \
  crown/spork.h \
  cuckoocache.h \
  dbwrapper.h \
  flat-database.h \
  flatfile.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  index/base.h \
  index/blockfilterindex.h \
  index/disktxpos.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  interfaces/chain.h \
  interfaces/handler.h \
  interfaces/node.h \
  interfaces/wallet.h \
  key.h \
  key_io.h \
  leveldbwrapper.h \
  logging.h \
  logging/timer.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  mn_processing.h \
  net.h \
  netfulfilledman.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  nodeconfig.h \
  node/coin.h \
  node/coinstats.h \
  node/context.h \
  node/psbt.h \
  node/transaction.h \
  node/ui_interface.h \
  node/utxo_snapshot.h \
  noui.h \
  optional.h \
  outputtype.h \
  platform/specialtx-common.h \
  platform/specialtx.h \
  platform/platform-db.h \
  platform/platform-utils.h \
  platform/nf-token/nf-token-index.h \
  platform/nf-token/nf-token-multiindex-utils.h \
  platform/nf-token/nf-token-protocol-index.h \
  platform/nf-token/nf-token-protocol-reg-tx.h \
  platform/nf-token/nf-token-protocol-tx-mem-pool-handler.h \
  platform/nf-token/nf-token-protocol.h \
  platform/nf-token/nf-token-reg-tx-builder.h \
  platform/nf-token/nf-token-reg-tx.h \
  platform/nf-token/nf-token-tx-mem-pool-handler.h \
  platform/nf-token/nf-token.h \
  platform/nf-token/nf-tokens-manager.h \
  platform/nf-token/nft-protocol-reg-tx-builder.h \
  platform/nf-token/nft-protocols-manager.h \
  policy/feerate.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  policy/settings.h \
  pow.h \
  protocol.h \
  psbt.h \
  random.h \
  randomenv.h \
  reverse_iterator.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/util.h \
  scheduler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/sigcache.h \
  script/sign.h \
  script/signingprovider.h \
  script/standard.h \
  shutdown.h \
  signet.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  pos/blockwitness.h \
  pos/kernel.h \
  pos/prooftracker.h \
  pos/stakepointer.h \
  pos/stakeminer.h \
  pos/stakevalidation.h \
  nodeconfig.h \
  threadinterrupt.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txrequest.h \
  txmempool.h \
  undo.h \
  util/asmap.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/check.h \
  util/error.h \
  util/fees.h \
  util/golombrice.h \
  util/macros.h \
  util/memory.h \
  util/message.h \
  util/moneystr.h \
  util/rbf.h \
  util/ref.h \
  util/settings.h \
  util/spanparsing.h \
  util/string.h \
  util/system.h \
  util/threadnames.h \
  util/time.h \
  util/translation.h \
  util/ui_change_type.h \
  util/url.h \
  util/vector.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  versionbitsinfo.h \
  wallet/bdb.h \
  wallet/coincontrol.h \
  wallet/coinselection.h \
  wallet/context.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/ismine.h \
  wallet/load.h \
  wallet/rpcwallet.h \
  wallet/salvage.h \
  wallet/scriptpubkeyman.h \
  wallet/sqlite.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  walletinitinterface.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h \
  zmq/zmqutil.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libcrown_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between crownd and crown-qt
# Contains code accessing mempool and chain state that is meant to be separated
# from wallet and gui code (see node/README.md). Shared code should go in
# libcrown_common or libcrown_util libraries, instead.
libcrown_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libcrown_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_server_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  banman.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  consensus/tx_verify.cpp \
  crown/cache.cpp \
  crown/init.cpp \
  crown/instantx.cpp \
  crown/legacycalls.cpp \
  crown/legacysigner.cpp \
  crown/nodesync.cpp \
  crown/nodewallet.cpp \
  crown/spork.cpp \
  dbwrapper.cpp \
  flatfile.cpp \
  dbdetails.cpp \
  dbdetails.h \
  httprpc.cpp \
  httpserver.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/txindex.cpp \
  init.cpp \
  interfaces/chain.cpp \
  interfaces/node.cpp \
  masternode/activemasternode.cpp \
  masternode/activemasternode.h \
  masternode/masternode-budget.cpp \
  masternode/masternode-budget.h \
  masternode/masternodeconfig.cpp \
  masternode/masternodeconfig.h \
  masternode/masternode.cpp \
  masternode/masternode.h \
  masternode/masternodeman.cpp \
  masternode/masternodeman.h \
  masternode/masternode-payments.cpp \
  masternode/masternode-payments.h \
  masternode/masternode-sync.cpp \
  masternode/masternode-sync.h \
  miner.cpp \
  mn_processing.cpp \
  pos/kernel.cpp \
  pos/prooftracker.cpp \
  pos/stakeminer.cpp \
  pos/stakepointer.cpp \
  pos/stakevalidation.cpp \
  net.cpp \
  netfulfilledman.cpp \
  net_processing.cpp \
  nodeconfig.cpp \
  node/coin.cpp \
  node/coinstats.cpp \
  node/context.cpp \
  node/psbt.cpp \
  node/transaction.cpp \
  node/ui_interface.cpp \
  noui.cpp \
  platform/agent.cpp \
  platform/agent.h \
  platform/governance.cpp \
  platform/governance.h \
  platform/governance-approval-voting.h \
  platform/governance-approval-voting.cpp \
  platform/governance-vote.cpp \
  platform/governance-vote.h \
  platform/specialtx.cpp \
  platform/platform-db.cpp \
  platform/nf-token/nf-token-multiindex-utils.cpp \
  platform/nf-token/nf-token-protocol-reg-tx.cpp \
  platform/nf-token/nf-token-protocol-tx-mem-pool-handler.cpp \
  platform/nf-token/nf-token-protocol.cpp \
  platform/nf-token/nf-token-reg-tx.cpp \
  platform/nf-token/nf-token-tx-mem-pool-handler.cpp \
  platform/nf-token/nf-tokens-manager.cpp \
  platform/nf-token/nft-protocols-manager.cpp \
  policy/fees.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/budget.cpp \
  rpc/masternode.cpp \
  rpc/systemnode.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  signet.cpp \
  systemnode/activesystemnode.cpp \
  systemnode/activesystemnode.h \
  systemnode/systemnodeconfig.cpp \
  systemnode/systemnodeconfig.h \
  systemnode/systemnode.cpp \
  systemnode/systemnode.h \
  systemnode/systemnodeman.cpp \
  systemnode/systemnodeman.h \
  systemnode/systemnode-payments.cpp \
  systemnode/systemnode-payments.h \
  systemnode/systemnode-sync.cpp \
  systemnode/systemnode-sync.h \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txrequest.cpp \
  txmempool.cpp \
  leveldbwrapper.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(CROWN_CORE_H)

if ENABLE_WALLET
libcrown_server_a_SOURCES += wallet/init.cpp
endif
if !ENABLE_WALLET
libcrown_server_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
libcrown_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES) $(ZMQ_CFLAGS)
libcrown_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp \
  zmq/zmqutil.cpp
endif


# wallet: shared between crownd and crown-qt, but only linked
# when wallet enabled
libcrown_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES) $(SQLITE_CFLAGS)
libcrown_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_wallet_a_SOURCES = \
  interfaces/wallet.cpp \
  wallet/bdb.cpp \
  wallet/coincontrol.cpp \
  wallet/context.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/load.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/salvage.cpp \
  wallet/scriptpubkeyman.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(CROWN_CORE_H)

if USE_SQLITE
libcrown_wallet_a_SOURCES += wallet/sqlite.cpp
endif

libcrown_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES)
libcrown_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(CROWN_CORE_H)

# crypto primitives library
crypto_libcrown_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libcrown_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libcrown_crypto_base_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha_poly_aead.h \
  crypto/chacha_poly_aead.cpp \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hkdf_sha256_32.cpp \
  crypto/hkdf_sha256_32.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/poly1305.h \
  crypto/poly1305.cpp \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha3.cpp \
  crypto/sha3.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h

if USE_ASM
crypto_libcrown_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
endif

crypto_libcrown_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libcrown_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libcrown_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libcrown_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
crypto_libcrown_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

crypto_libcrown_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libcrown_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libcrown_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libcrown_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
crypto_libcrown_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

crypto_libcrown_crypto_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libcrown_crypto_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libcrown_crypto_shani_a_CXXFLAGS += $(SHANI_CXXFLAGS)
crypto_libcrown_crypto_shani_a_CPPFLAGS += -DENABLE_SHANI
crypto_libcrown_crypto_shani_a_SOURCES = crypto/sha256_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
libcrown_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES)
libcrown_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/tx_check.cpp \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/pureheader.cpp \
  primitives/pureheader.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/crownconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  span.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  util/strencodings.cpp \
  util/strencodings.h \
  version.h

# common: shared between crownd, and crown-qt and non-server tools
libcrown_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES)
libcrown_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_common_a_SOURCES = \
  auxpow.cpp \
  base58.cpp \
  bech32.cpp \
  bloom.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  key_io.cpp \
  merkleblock.cpp \
  netaddress.cpp \
  netbase.cpp \
  net_permissions.cpp \
  outputtype.cpp \
  policy/feerate.cpp \
  policy/policy.cpp \
  protocol.cpp \
  psbt.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/util.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  versionbitsinfo.cpp \
  warnings.cpp \
  $(CROWN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libcrown_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES)
libcrown_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  interfaces/handler.cpp \
  logging.cpp \
  random.cpp \
  randomenv.cpp \
  rpc/request.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/asmap.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/system.cpp \
  util/message.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/settings.cpp \
  util/threadnames.cpp \
  util/spanparsing.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/time.cpp \
  $(CROWN_CORE_H)

if USE_LIBEVENT
libcrown_util_a_SOURCES += util/url.cpp
endif

if GLIBC_BACK_COMPAT
libcrown_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between crown-cli and crown-qt
libcrown_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES)
libcrown_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libcrown_cli_a_SOURCES = \
  compat/stdin.h \
  compat/stdin.cpp \
  rpc/client.cpp \
  $(CROWN_CORE_H)

nodist_libcrown_util_a_SOURCES = $(srcdir)/obj/build.h
#

# crownd & crown-node binaries #
crown_daemon_sources = crownd.cpp
crown_bin_cppflags = $(AM_CPPFLAGS) $(CROWN_INCLUDES)
crown_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
crown_bin_ldflags = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
crown_daemon_sources += crownd-res.rc
endif

crown_bin_ldadd = \
  $(LIBCROWN_WALLET) \
  $(LIBCROWN_COMMON) \
  $(LIBCROWN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBCROWN_ZMQ) \
  $(LIBCROWN_CONSENSUS) \
  $(LIBCROWN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

crown_bin_ldadd += $(BOOST_LIBS) $(BDB_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS)

crownd_SOURCES = $(crown_daemon_sources)
crownd_CPPFLAGS = $(crown_bin_cppflags)
crownd_CXXFLAGS = $(crown_bin_cxxflags)
crownd_LDFLAGS = $(crown_bin_ldflags)
crownd_LDADD = $(LIBCROWN_SERVER) $(crown_bin_ldadd)

crown_node_SOURCES = $(crown_daemon_sources)
crown_node_CPPFLAGS = $(crown_bin_cppflags)
crown_node_CXXFLAGS = $(crown_bin_cxxflags)
crown_node_LDFLAGS = $(crown_bin_ldflags)
crown_node_LDADD = $(LIBCROWN_SERVER) $(crown_bin_ldadd)

# crown-cli binary #
crown_cli_SOURCES = crown-cli.cpp
crown_cli_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES) $(EVENT_CFLAGS)
crown_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crown_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
crown_cli_SOURCES += crown-cli-res.rc
endif

crown_cli_LDADD = \
  $(LIBCROWN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBCROWN_UTIL) \
  $(LIBCROWN_CRYPTO)

crown_cli_LDADD += $(BOOST_LIBS) $(EVENT_LIBS)
#

# crown-tx binary #
crown_tx_SOURCES = crown-tx.cpp
crown_tx_CPPFLAGS = $(AM_CPPFLAGS) $(CROWN_INCLUDES)
crown_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crown_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
crown_tx_SOURCES += crown-tx-res.rc
endif

crown_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBCROWN_COMMON) \
  $(LIBCROWN_UTIL) \
  $(LIBCROWN_CONSENSUS) \
  $(LIBCROWN_CRYPTO) \
  $(LIBSECP256K1)

crown_tx_LDADD += $(BOOST_LIBS)
#

# crown-wallet binary #
crown_wallet_SOURCES = crown-wallet.cpp
crown_wallet_CPPFLAGS = $(crown_bin_cppflags)
crown_wallet_CXXFLAGS = $(crown_bin_cxxflags)
crown_wallet_LDFLAGS = $(crown_bin_ldflags)
crown_wallet_LDADD = $(LIBCROWN_WALLET_TOOL) $(crown_bin_ldadd)

crown_wallet_LDADD += \
  $(LIBCROWN_SERVER) \
  $(LIBCROWN_WALLET) \
  $(LIBCROWN_COMMON) \
  $(LIBCROWN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBCROWN_ZMQ) \
  $(LIBCROWN_CONSENSUS) \
  $(LIBCROWN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

crown_wallet_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(MINIUPNPC_LIBS) $(ZMQ_LIBS)

if TARGET_WINDOWS
crown_wallet_SOURCES += crown-wallet-res.rc
endif
#

# crownconsensus library #
if BUILD_CROWN_LIBS
include_HEADERS = script/crownconsensus.h
libcrownconsensus_la_SOURCES = support/cleanse.cpp $(crypto_libcrown_crypto_base_a_SOURCES) $(libcrown_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libcrownconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libcrownconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libcrownconsensus_la_LIBADD = $(LIBSECP256K1)
libcrownconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_CROWN_INTERNAL
libcrownconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crc32c/src/*.gcda crc32c/src/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += crown/*.gcda crown/*.gcno
CLEANFILES += masternode/*.gcda masternode/*.gcno
CLEANFILES += systemnode/*.gcda systemnode/*.gcno
CLEANFILES += pos/*.gcda pos/*.gcno
CLEANFILES += index/*.gcda index/*.gcno
CLEANFILES += interfaces/*.gcda interfaces/*.gcno
CLEANFILES += node/*.gcda node/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += rpc/*.gcda rpc/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += util/*.gcda util/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/crown-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/crown-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/crown-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/crown-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if TARGET_DARWIN
	@echo "Checking macOS dynamic libraries..."
	$(AM_V_at) OTOOL=$(OTOOL) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_WINDOWS
	@echo "Checking Windows dynamic libraries..."
	$(AM_V_at) OBJDUMP=$(OBJDUMP) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) OTOOL=$(OTOOL) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py $(bin_PROGRAMS)
endif

if EMBEDDED_LEVELDB
include Makefile.crc32c.include
include Makefile.leveldb.include
endif

include Makefile.test_util.include
include Makefile.test_fuzz.include

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
