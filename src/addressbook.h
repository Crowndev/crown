// Copyright (c) 2019 The CROWN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_ADDRESSBOOK_H
#define CROWN_ADDRESSBOOK_H

#include <map>
#include <string>

enum class AddressBookPurpose{UNKNOWN, RECEIVE, SEND};
std::string GetAddressBookPurposeType(AddressBookPurpose t);

/** Address book data */
class CAddressBookData {
	private:
    bool m_change{true};
    std::string m_label;
public:
	std::string name;
	std::string purpose;
	
	CAddressBookData() {
		purpose = GetAddressBookPurposeType(AddressBookPurpose::UNKNOWN);
	}

	typedef std::map<std::string, std::string> StringMap;
	StringMap destdata;

	bool isSendPurpose() const;
	bool isReceivePurpose() const;
    bool IsChange() const { return m_change; }
    const std::string& GetLabel() const { return m_label; }
    void SetLabel(const std::string& label) {
        m_change = false;
        m_label = label;
    }
};
#endif //CROWN_ADDRESSBOOK_H
