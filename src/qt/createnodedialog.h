#ifndef CREATENODEDIALOG_H
#define CREATENODEDIALOG_H

#include <QDialog>

class WalletModel;
class PlatformStyle;
class SendCollateralDialog;
namespace Ui {
class CreateNodeDialog;
}

class CreateNodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateNodeDialog(const PlatformStyle *_platformStyle, QWidget *parent = 0);
    ~CreateNodeDialog();

public:
    QString getAlias();
    QString getIP();
    QString getLabel();
    void setAlias(QString alias);
    void setIP(QString ip);
    void setNoteLabel(QString text);
    void setEditMode();
    void setWalletModel(WalletModel* model);
    void setMode(int m);
protected Q_SLOTS:
    void accept() override;

private:
    virtual bool aliasExists(QString alias) = 0;
    bool CheckAlias();
    bool CheckIP();
    bool editMode;
    QString startAlias;
    WalletModel *walletmodel = nullptr;
    const PlatformStyle *platformStyle;
    int mode;   
    SendCollateralDialog *sendDialog;
    
    Ui::CreateNodeDialog *ui;
};

#endif // CREATENODEDIALOG_H
