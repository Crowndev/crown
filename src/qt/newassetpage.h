#ifndef NEWASSETPAGE_H
#define NEWASSETPAGE_H

#include <QDialog>

namespace Ui {
class NewAssetPage;
}

class NewAssetPage : public QDialog
{
    Q_OBJECT

public:
    explicit NewAssetPage(QWidget *parent = nullptr);
    ~NewAssetPage();

	QString getinputamount();
	QString outputamount();
	QString getassettype();
	QString getassetcontract();
	bool gettransferable();
	bool getconvertable();
	bool getrestricted();
	bool getlimited();
	QString getexpiry();

private:
    Ui::NewAssetPage *ui;
};

#endif // NEWASSETPAGE_H
