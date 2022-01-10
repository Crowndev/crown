// Copyright (c) 2011-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_QT_CROWNADDRESSVALIDATOR_H
#define CROWN_QT_CROWNADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class CrownAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit CrownAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Crown address widget validator, checks for a valid crown address.
 */
class CrownAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit CrownAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // CROWN_QT_CROWNADDRESSVALIDATOR_H
