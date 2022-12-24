// Copyright (c) 2019 The CROWN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addressbook.h>
#include <string>

bool CAddressBookData::isSendPurpose() const {
	return purpose == GetAddressBookPurposeType(AddressBookPurpose::SEND);
}

bool CAddressBookData::isReceivePurpose() const {
	return purpose == GetAddressBookPurposeType(AddressBookPurpose::RECEIVE);
}

std::string GetAddressBookPurposeType(AddressBookPurpose t){
    switch (t) {
        case AddressBookPurpose::UNKNOWN: return "unknown";
        case AddressBookPurpose::RECEIVE: return "receive";
        case AddressBookPurpose::SEND: return "send";
    }
    return "";
}
