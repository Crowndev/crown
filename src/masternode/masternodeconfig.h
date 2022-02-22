// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_MASTERNODECONFIG_H_
#define SRC_MASTERNODECONFIG_H_

#include <string>
#include <vector>
#include <nodeconfig.h>

#include <util/system.h>

class CMasternodeConfig;
extern CMasternodeConfig masternodeConfig;

class CMasternodeConfig : public CNodeConfig
{
private:
    fs::path getNodeConfigFile() override;
    std::string getHeader() override;
    std::string getFileName() override;
};

#endif /* SRC_MASTERNODECONFIG_H_ */
