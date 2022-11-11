#ifndef CREATEMASTERNODEDIALOG_H
#define CREATEMASTERNODEDIALOG_H

#include <qt/createnodedialog.h>
#include <masternode/masternodeconfig.h>

#include <QDialog>

class CreateMasternodeDialog : public CreateNodeDialog
{
    Q_OBJECT

public:
    explicit CreateMasternodeDialog(const PlatformStyle *_platformStyle, QWidget *parent = 0)
        : CreateNodeDialog(_platformStyle, parent)
    {
        setWindowTitle("Create a new Masternode");
    }
private:
    virtual bool aliasExists(QString alias) override
    {
        for (auto& l : masternodeConfig.getEntries()) {
           if (l.getAlias() == alias.toStdString())
               return true;
        }
        return false;
    }
};

#endif // CREATEMASTERNODEDIALOG_H
