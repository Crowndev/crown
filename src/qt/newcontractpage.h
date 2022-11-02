#ifndef NEWCONTRACTPAGE_H
#define NEWCONTRACTPAGE_H

#include <QDialog>

namespace Ui {
class NewContractPage;
}

class NewContractPage : public QDialog
{
    Q_OBJECT

public:
    explicit NewContractPage(QWidget *parent = nullptr);
    ~NewContractPage();

     QString getName();
     QString getSymbol();
     QString getAddress();
     QString getwebsiteUrl();
     QString getcontractURL();
     QString getdescription();
     QString getscript();

private:
    Ui::NewContractPage *ui;
};

#endif // NEWCONTRACTPAGE_H
