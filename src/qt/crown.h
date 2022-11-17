// Copyright (c) 2011-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_QT_CROWN_H
#define CROWN_QT_CROWN_H

#if defined(HAVE_CONFIG_H)
#include <config/crown-config.h>
#endif

#include <QApplication>
#include <QDebug>
#include <QThread>
#include <assert.h>
#include <memory>

#include <interfaces/node.h>

class CrownGUI;
class ClientModel;
class NetworkStyle;
class OptionsModel;
class PaymentServer;
class PlatformStyle;
class SplashScreen;
class WalletController;
class WalletModel;


/** Class encapsulating Crown Core startup and shutdown.
 * Allows running startup and shutdown in a different thread from the UI thread.
 */
class CrownCore: public QObject
{
    Q_OBJECT
public:
    explicit CrownCore(interfaces::Node& node);

public Q_SLOTS:
    void initialize();
    void shutdown();

Q_SIGNALS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    void runawayException(const QString &message);

private:
    /// Pass fatal exception message to UI thread
    void handleRunawayException(const std::exception *e);

    interfaces::Node& m_node;
};

/** Main Crown application object */
class CrownApplication: public QApplication
{
    Q_OBJECT
public:
    explicit CrownApplication();
    ~CrownApplication();

#ifdef ENABLE_WALLET
    /// Create payment server
    void createPaymentServer();
#endif
    /// parameter interaction/setup based on rules
    void parameterSetup();
    /// Create options model
    void createOptionsModel(bool resetSettings);
    /// Initialize prune setting
    void InitializePruneSetting(bool prune);
    /// Create main window
    void createWindow(const NetworkStyle *networkStyle);
    /// Create splash screen
    void createSplashScreen(const NetworkStyle *networkStyle);
    /// Basic initialization, before starting initialization/shutdown thread. Return true on success.
    bool baseInitialize();

    /// Request core initialization
    void requestInitialize();
    /// Request core shutdown
    void requestShutdown();

    /// Get process return value
    int getReturnValue() const { return returnValue; }

    /// Get window identifier of QMainWindow (CrownGUI)
    WId getMainWinId() const;

    /// Setup platform style
    void setupPlatformStyle();

    interfaces::Node& node() const { assert(m_node); return *m_node; }
    void setNode(interfaces::Node& node);

    bool notify(QObject* receiver, QEvent *e)
    {
        try {
            return QApplication::notify(receiver, e);
        }
        catch(std::runtime_error e)
        {
            qDebug() << "std::runtime_error in thread : " << QThread::currentThreadId();
            qDebug() << e.what();
        }
        catch(std::exception e)
        {
            qDebug() << "std::exception in thread : " << QThread::currentThreadId();
            qDebug() << e.what();
        }
        catch(...)
        {
            qDebug() << "exception thread : " << QThread::currentThreadId();
        }

        return false;
    }


public Q_SLOTS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    /// Handle runaway exceptions. Shows a message box with the problem and quits the program.
    void handleRunawayException(const QString &message);

Q_SIGNALS:
    void requestedInitialize();
    void requestedShutdown();
    void splashFinished();
    void windowShown(CrownGUI* window);

private:
    QThread *coreThread;
    OptionsModel *optionsModel;
    ClientModel *clientModel;
    CrownGUI *window;
    QTimer *pollShutdownTimer;
#ifdef ENABLE_WALLET
    PaymentServer* paymentServer{nullptr};
    WalletController* m_wallet_controller{nullptr};
#endif
    int returnValue;
    const PlatformStyle *platformStyle;
    std::unique_ptr<QWidget> shutdownWindow;
    SplashScreen* m_splash = nullptr;
    interfaces::Node* m_node = nullptr;

    void startThread();
};

int GuiMain(int argc, char* argv[]);

#endif // CROWN_QT_CROWN_H
