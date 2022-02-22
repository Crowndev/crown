// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_SYSTEMNODECONFIG_H_
#define SRC_SYSTEMNODECONFIG_H_

#include <string>
#include <vector>
#include <nodeconfig.h>

#include <util/system.h>


class CSystemnodeConfig;
extern CSystemnodeConfig systemnodeConfig;

class CSystemnodeConfig : public CNodeConfig
{
private:
    fs::path getNodeConfigFile() override;
    std::string getHeader() override;
    std::string getFileName() override;
};

#endif /* SRC_SYSTEMNODECONFIG_H_ */
