#ifndef CREATESYSTEMNODEDIALOG_H
#define CREATESYSTEMNODEDIALOG_H

#include <qt/createnodedialog.h>
#include <systemnode/systemnodeconfig.h>

#include <QDialog>

class CreateSystemnodeDialog : public CreateNodeDialog
{
    Q_OBJECT

public:
    explicit CreateSystemnodeDialog(const PlatformStyle *_platformStyle, QWidget *parent = 0)
        : CreateNodeDialog(_platformStyle, parent)
    {
        setWindowTitle("Create a new Systemnode");
    }
private:
    virtual bool aliasExists(QString alias) override
    {
        for (auto& l : systemnodeConfig.getEntries()) {
           if (l.getAlias() == alias.toStdString())
               return true;
        }
        return false;
    }
};

#endif // CREATESYSTEMNODEDIALOG_H
